#include "trace_reader.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <zlib.h>
#include <lzma.h>
#include <zstd.h>

// In-process streaming decoder for ChampSim trace files.
//
// Replaces the previous popen("gunzip|xz|zstd -dc FILE") pipeline with a
// direct file descriptor + library-level streaming decode. The motivation
// is NFS: popen'd decompressors read with small block sizes and serialize
// RPCs, which becomes the dominant cost when many Slurm array jobs share
// a node. Going in-process lets us:
//
//   * issue large reads (8 MiB) against the file, well above any plausible
//     NFS rsize, so each kernel-side request becomes a coherent batch of
//     parallel RPCs;
//   * advise the kernel of access pattern (POSIX_FADV_SEQUENTIAL) to
//     widen the readahead window;
//   * pipeline the next chunk via POSIX_FADV_WILLNEED while the decoder
//     is busy on the current one;
//   * release already-consumed pages (POSIX_FADV_DONTNEED) so 16-32 jobs
//     on the same node do not evict each other's working sets.
//
// Format dispatch is by file extension, matching the historical behavior:
// .gz -> zlib (with auto-header so .zlib also works), .xz -> liblzma,
// .zst / .zstd -> libzstd. Each backend implements the same Decoder
// interface; TraceReader owns one and drives it through Impl.

namespace
{

// 8 MiB compressed read buffer. Past ~4-8 MiB the NFS readahead window is
// saturated; bigger buffers waste resident memory across the typical
// 16-32 jobs/node fanout without further reducing wire RPC count.
constexpr std::size_t COMP_BUF_BYTES = 8 * 1024 * 1024;

// 1 MiB decompressed scratch. Doesn't touch NFS, just amortizes the
// per-call overhead of the decoder against the simulator's 64-byte
// record reads.
constexpr std::size_t DECOMP_BUF_BYTES = 1 * 1024 * 1024;

// Drop already-read pages once the read cursor is at least this far past
// the previous DONTNEED point. Lagging by 2x the read buffer keeps very
// recently-touched pages in cache (in case the decoder seeks back inside
// a chunk) while still bounding cache footprint per process.
constexpr off_t DONTNEED_LAG_BYTES = 2 * static_cast<off_t>(COMP_BUF_BYTES);

// --- decoder strategy --------------------------------------------------

// Result codes from one decoder step. The caller cares about three
// outcomes: progress was made (Ok), no more bytes will ever come out
// (StreamEnd), or the input is corrupt (Error).
enum class DecodeStatus { Ok, StreamEnd, Error };

class Decoder
{
  public:
  virtual ~Decoder() = default;

  // Feed up to src_len bytes from src and write up to dst_len bytes to
  // dst. After return, *consumed and *produced report how many bytes
  // were actually used in each direction.
  virtual DecodeStatus decode(const std::uint8_t *src, std::size_t src_len,
                              std::size_t *consumed, std::uint8_t *dst,
                              std::size_t dst_len, std::size_t *produced) = 0;

  // Reset to the start of a fresh stream (used by TraceReader::rewind).
  virtual void reset() = 0;
};

class GzipDecoder : public Decoder
{
  z_stream s_{};
  bool     ended_ = false;

  void init_()
  {
    std::memset(&s_, 0, sizeof(s_));
    // 15 is the max window bits; +32 enables auto-detection of zlib/gzip
    // headers, which keeps us tolerant of either container.
    if (inflateInit2(&s_, 15 + 32) != Z_OK)
      throw std::runtime_error("TraceReader: zlib inflateInit2 failed");
  }

  public:
  GzipDecoder() { init_(); }
  ~GzipDecoder() override { inflateEnd(&s_); }

  DecodeStatus decode(const std::uint8_t *src, std::size_t src_len,
                      std::size_t *consumed, std::uint8_t *dst,
                      std::size_t dst_len, std::size_t *produced) override
  {
    s_.next_in   = const_cast<Bytef *>(src);
    s_.avail_in  = static_cast<uInt>(src_len);
    s_.next_out  = dst;
    s_.avail_out = static_cast<uInt>(dst_len);

    int rc      = inflate(&s_, Z_NO_FLUSH);
    *consumed   = src_len - s_.avail_in;
    *produced   = dst_len - s_.avail_out;

    if (rc == Z_STREAM_END) {
      ended_ = true;
      return DecodeStatus::StreamEnd;
    }
    // Z_BUF_ERROR just means "no progress without more input or output
    // space" — that's a normal stall, not an error.
    if (rc == Z_OK || rc == Z_BUF_ERROR)
      return DecodeStatus::Ok;
    return DecodeStatus::Error;
  }

  void reset() override
  {
    inflateEnd(&s_);
    ended_ = false;
    init_();
  }
};

class XzDecoder : public Decoder
{
  lzma_stream s_     = LZMA_STREAM_INIT;
  bool        ended_ = false;

  void init_()
  {
    s_ = LZMA_STREAM_INIT;
    // UINT64_MAX memlimit (no cap), CONCATENATED to handle multi-stream
    // .xz files transparently.
    if (lzma_stream_decoder(&s_, UINT64_MAX, LZMA_CONCATENATED) != LZMA_OK)
      throw std::runtime_error("TraceReader: lzma_stream_decoder failed");
  }

  public:
  XzDecoder() { init_(); }
  ~XzDecoder() override { lzma_end(&s_); }

  DecodeStatus decode(const std::uint8_t *src, std::size_t src_len,
                      std::size_t *consumed, std::uint8_t *dst,
                      std::size_t dst_len, std::size_t *produced) override
  {
    s_.next_in   = src;
    s_.avail_in  = src_len;
    s_.next_out  = dst;
    s_.avail_out = dst_len;

    lzma_ret rc = lzma_code(&s_, LZMA_RUN);
    *consumed   = src_len - s_.avail_in;
    *produced   = dst_len - s_.avail_out;

    if (rc == LZMA_STREAM_END) {
      ended_ = true;
      return DecodeStatus::StreamEnd;
    }
    if (rc == LZMA_OK)
      return DecodeStatus::Ok;
    return DecodeStatus::Error;
  }

  void reset() override
  {
    lzma_end(&s_);
    ended_ = false;
    init_();
  }
};

class ZstdDecoder : public Decoder
{
  ZSTD_DCtx *ctx_ = nullptr;

  public:
  ZstdDecoder()
  {
    ctx_ = ZSTD_createDCtx();
    if (ctx_ == nullptr)
      throw std::runtime_error("TraceReader: ZSTD_createDCtx failed");
  }
  ~ZstdDecoder() override
  {
    if (ctx_ != nullptr)
      ZSTD_freeDCtx(ctx_);
  }

  DecodeStatus decode(const std::uint8_t *src, std::size_t src_len,
                      std::size_t *consumed, std::uint8_t *dst,
                      std::size_t dst_len, std::size_t *produced) override
  {
    ZSTD_inBuffer  in  = {src, src_len, 0};
    ZSTD_outBuffer out = {dst, dst_len, 0};

    std::size_t rc = ZSTD_decompressStream(ctx_, &out, &in);
    *consumed      = in.pos;
    *produced      = out.pos;

    if (ZSTD_isError(rc))
      return DecodeStatus::Error;
    // Return value 0 indicates a frame is fully decoded. ChampSim trace
    // files are single-frame, so we surface this as stream end. (For a
    // multi-frame input we would need to keep going — not relevant here.)
    if (rc == 0)
      return DecodeStatus::StreamEnd;
    return DecodeStatus::Ok;
  }

  void reset() override
  {
    // session_only resets stream state without freeing tables; cheap.
    ZSTD_DCtx_reset(ctx_, ZSTD_reset_session_only);
  }
};

std::unique_ptr<Decoder> make_decoder(const std::string &path)
{
  // Same first-character-after-last-dot check as the original code.
  const auto dot = path.rfind('.');
  if (dot == std::string::npos || dot + 1 >= path.size())
    throw std::runtime_error(
      "TraceReader: trace file has no extension: " + path);

  const char tag = path[dot + 1];
  if (tag == 'g')
    return std::unique_ptr<Decoder>(new GzipDecoder());
  if (tag == 'x')
    return std::unique_ptr<Decoder>(new XzDecoder());
  if (tag == 'z')
    return std::unique_ptr<Decoder>(new ZstdDecoder());

  throw std::runtime_error(
    "TraceReader: unsupported compression (expected .gz/.xz/.zst): " + path);
}

}  // namespace

// --- TraceReader::Impl -------------------------------------------------

struct TraceReader::Impl {
  int fd = -1;

  std::vector<std::uint8_t> comp_buf;
  std::vector<std::uint8_t> decomp_buf;

  // [comp_pos, comp_size) is the unread portion of comp_buf.
  std::size_t comp_pos  = 0;
  std::size_t comp_size = 0;

  // [decomp_pos, decomp_size) is the unread portion of decomp_buf.
  std::size_t decomp_pos  = 0;
  std::size_t decomp_size = 0;

  // Total bytes ever read from the fd (for fadvise offset arithmetic).
  off_t file_offset      = 0;
  off_t dontneed_cursor  = 0;

  bool file_eof      = false;
  bool stream_ended  = false;

  std::unique_ptr<Decoder> decoder;

  Impl() : comp_buf(COMP_BUF_BYTES), decomp_buf(DECOMP_BUF_BYTES) {}

  ~Impl()
  {
    if (fd >= 0)
      ::close(fd);
  }

  void open_file(const std::string &path)
  {
    fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0)
      throw std::runtime_error("TraceReader: open(\"" + path
                               + "\") failed: " + std::strerror(errno));
    // SEQUENTIAL roughly doubles the kernel's NFS readahead window. This
    // is the single biggest fadvise lever for our access pattern.
    ::posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
  }

  void rewind_state()
  {
    if (fd < 0)
      throw std::runtime_error("TraceReader: rewind on closed fd");
    if (::lseek(fd, 0, SEEK_SET) == static_cast<off_t>(-1))
      throw std::runtime_error("TraceReader: lseek failed: "
                               + std::string(std::strerror(errno)));
    comp_pos = comp_size = 0;
    decomp_pos = decomp_size = 0;
    file_offset              = 0;
    dontneed_cursor          = 0;
    file_eof                 = false;
    stream_ended             = false;
    decoder->reset();
    ::posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
  }

  // Pull one chunk of compressed bytes from the file into comp_buf and
  // update fadvise hints around it. Idempotent past EOF.
  void refill_compressed()
  {
    if (file_eof) {
      comp_pos = comp_size = 0;
      return;
    }

    ssize_t n = ::read(fd, comp_buf.data(), comp_buf.size());
    if (n < 0)
      throw std::runtime_error("TraceReader: read failed: "
                               + std::string(std::strerror(errno)));

    comp_pos  = 0;
    comp_size = static_cast<std::size_t>(n);

    if (n == 0) {
      file_eof = true;
      return;
    }

    file_offset += n;

    // Tell the kernel to start fetching the chunk we'll want next, so
    // its NFS RPCs overlap with our decode work on the current chunk.
    ::posix_fadvise(fd, file_offset, COMP_BUF_BYTES, POSIX_FADV_WILLNEED);

    // Drop pages that are now well behind the read cursor. The lag
    // (DONTNEED_LAG_BYTES) keeps "just-touched" pages in cache so a
    // small backseek inside the decoder doesn't refault, while still
    // bounding our resident set when many jobs share a node.
    if (file_offset - dontneed_cursor >= DONTNEED_LAG_BYTES) {
      ::posix_fadvise(fd, dontneed_cursor,
                      file_offset - dontneed_cursor, POSIX_FADV_DONTNEED);
      dontneed_cursor = file_offset;
    }
  }

  // Push compressed bytes through the decoder until decomp_buf has at
  // least one byte of output, or we determine the stream is exhausted.
  // Returns false at end of stream, true otherwise.
  bool refill_decompressed()
  {
    decomp_pos  = 0;
    decomp_size = 0;

    while (decomp_size == 0) {
      if (stream_ended)
        return false;

      // If the compressed buffer is drained, pull another chunk. Note:
      // we still call decode() once afterwards even if the file just
      // hit EOF — the decoder may have buffered output to flush.
      if (comp_pos >= comp_size && !file_eof)
        refill_compressed();

      std::size_t consumed = 0, produced = 0;
      DecodeStatus st = decoder->decode(
        comp_buf.data() + comp_pos, comp_size - comp_pos, &consumed,
        decomp_buf.data() + decomp_size, decomp_buf.size() - decomp_size,
        &produced);

      if (st == DecodeStatus::Error)
        throw std::runtime_error("TraceReader: decode error in stream");

      comp_pos += consumed;
      decomp_size += produced;

      if (st == DecodeStatus::StreamEnd) {
        stream_ended = true;
        // Fall out of the loop on the next iteration if produced == 0,
        // or deliver what we just produced if produced > 0.
      }

      // Guard against an infinite loop: if the decoder neither consumed
      // nor produced anything, and we have no more bytes to feed, we're
      // genuinely done.
      if (consumed == 0 && produced == 0
          && comp_pos >= comp_size && file_eof) {
        stream_ended = true;
      }
    }
    return true;
  }

  // Copy out exactly nbytes of decompressed data, or return false if
  // the stream ends before nbytes are available. A short tail at EOF is
  // reported as failure (mirrors fread(..., size, 1, fp) == 1).
  bool read_bytes(void *dst, std::size_t nbytes)
  {
    auto       *out    = static_cast<std::uint8_t *>(dst);
    std::size_t copied = 0;

    while (copied < nbytes) {
      if (decomp_pos >= decomp_size) {
        if (!refill_decompressed())
          return false;
      }
      const std::size_t avail = decomp_size - decomp_pos;
      const std::size_t n     = std::min(avail, nbytes - copied);
      std::memcpy(out + copied, decomp_buf.data() + decomp_pos, n);
      decomp_pos += n;
      copied += n;
    }
    return true;
  }
};

// --- TraceReader (public) ---------------------------------------------

TraceReader::TraceReader(const std::string &path)
  : path_(path), impl_(new Impl())
{
  impl_->decoder = make_decoder(path);
  impl_->open_file(path);
}

TraceReader::~TraceReader() = default;

bool TraceReader::read(void *dst, std::size_t nbytes)
{
  return impl_->read_bytes(dst, nbytes);
}

void TraceReader::rewind() { impl_->rewind_state(); }

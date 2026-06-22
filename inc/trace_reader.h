#ifndef TRACE_READER_H
#define TRACE_READER_H

#include <cstddef>
#include <memory>
#include <string>

// TraceReader owns a single trace file and exposes a decompressed byte
// stream to the simulator. The compression format (gzip, xz, zstd) is
// detected from the file extension. All buffering, prefetch, and decoder
// state is encapsulated here so that ooo_cpu / main do not need to know
// how the bytes are produced.
//
// v1 implementation backs onto popen() of the appropriate decompressor,
// matching the historical behavior. A future revision swaps that for an
// in-process streaming decoder with posix_fadvise-driven NFS prefetch
// without changing this public API.
class TraceReader
{
  public:
  // Opens the trace at `path`. Throws std::runtime_error if the file is
  // missing/unreadable, has no recognizable compression extension, or if
  // the decompressor cannot be launched.
  explicit TraceReader(const std::string &path);
  ~TraceReader();

  TraceReader(const TraceReader &)            = delete;
  TraceReader &operator=(const TraceReader &) = delete;

  // Reads exactly `nbytes` of decompressed data into `dst`.
  // Returns true on success, false at end of stream (mirrors the
  // fread(..., size, 1, fp) == 1 idiom the simulator used previously).
  bool read(void *dst, std::size_t nbytes);

  // Resets the stream to the start of the trace. Used when the simulator
  // loops a trace after exhausting it.
  void rewind();

  const std::string &path() const noexcept { return path_; }

  private:
  struct Impl;

  std::string           path_;
  std::unique_ptr<Impl> impl_;
};

#endif  // TRACE_READER_H

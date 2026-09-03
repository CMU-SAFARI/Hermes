#ifndef ZSTD_FILE_H
#define ZSTD_FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <vector>
#include <zstd.h>

using namespace std;

// Streaming zstd I/O for the offline tools. Reads are exact-length, so a short
// final read is a clean EOF rather than a silently repeated record. EOF inside
// an unclosed frame is fatal, not a clean EOF: a run that died mid-trace would
// otherwise replay as a complete but shorter one.
class ZstdReader
{
private:
  FILE        *f    = NULL;
  ZSTD_DCtx   *dctx = NULL;
  vector<char> in, out;
  size_t       in_pos = 0, in_size = 0, out_pos = 0, out_size = 0;
  size_t       frame_rc  = 0;  // non-zero: mid-frame, zstd expects more input
  bool         saw_input = false;
  string       filename;

  bool refill();

public:
  void open(const string &path);
  bool read(void *dst, size_t len);
  void close();
};

class ZstdWriter
{
private:
  FILE        *f    = NULL;
  ZSTD_CCtx   *cctx = NULL;
  vector<char> in, out;

  void drain(ZSTD_EndDirective mode);

public:
  void open(const string &path);
  void write(const void *src, size_t len);
  void close();
};

void ZstdReader::open(const string &path)
{
  filename = path;
  f        = fopen(path.c_str(), "rb");
  assert(f != NULL);
  dctx = ZSTD_createDCtx();
  assert(dctx != NULL);
  in.resize(ZSTD_DStreamInSize());
  out.resize(ZSTD_DStreamOutSize());
}

// Decompresses one more chunk; false once the stream is exhausted.
bool ZstdReader::refill()
{
  while (out_pos == out_size) {
    if (in_pos == in_size) {
      in_size = fread(&in[0], 1, in.size(), f);
      in_pos  = 0;
      if (in_size == 0) {
        if (frame_rc != 0 || !saw_input) {
          fprintf(stderr,
                  "zstd_file: %s is truncated (%s). Refusing to replay it as a "
                  "complete trace.\n",
                  filename.c_str(),
                  saw_input ? "unterminated frame" : "no data");
          exit(1);
        }
        return false;
      }
      saw_input = true;
    }
    ZSTD_inBuffer  zin  = {&in[0], in_size, in_pos};
    ZSTD_outBuffer zout = {&out[0], out.size(), 0};
    size_t         rc   = ZSTD_decompressStream(dctx, &zout, &zin);
    assert(!ZSTD_isError(rc));
    frame_rc = rc;
    in_pos   = zin.pos;
    out_pos  = 0;
    out_size = zout.pos;
  }
  return true;
}

bool ZstdReader::read(void *dst, size_t len)
{
  char *d = (char *)dst;
  while (len > 0) {
    if (!refill()) {
      return false;
    }
    size_t n = out_size - out_pos;
    if (n > len) {
      n = len;
    }
    memcpy(d, &out[out_pos], n);
    out_pos += n;
    d += n;
    len -= n;
  }
  return true;
}

void ZstdReader::close()
{
  if (dctx != NULL) {
    ZSTD_freeDCtx(dctx);
    dctx = NULL;
  }
  if (f != NULL) {
    fclose(f);
    f = NULL;
  }
}

void ZstdWriter::open(const string &path)
{
  f = fopen(path.c_str(), "wb");
  assert(f != NULL);
  cctx = ZSTD_createCCtx();
  assert(cctx != NULL);
  out.resize(ZSTD_CStreamOutSize());
  in.reserve(ZSTD_CStreamInSize());
}

// Compresses everything staged. ZSTD_e_end also closes the frame.
void ZstdWriter::drain(ZSTD_EndDirective mode)
{
  ZSTD_inBuffer zin = {in.empty() ? NULL : &in[0], in.size(), 0};
  size_t        remaining;

  do {
    ZSTD_outBuffer zout = {&out[0], out.size(), 0};
    remaining           = ZSTD_compressStream2(cctx, &zout, &zin, mode);
    assert(!ZSTD_isError(remaining));
    fwrite(&out[0], 1, zout.pos, f);
  } while (mode == ZSTD_e_end ? remaining != 0 : zin.pos != zin.size);

  in.clear();
}

void ZstdWriter::write(const void *src, size_t len)
{
  const char *p = (const char *)src;
  in.insert(in.end(), p, p + len);
  if (in.size() >= ZSTD_CStreamInSize()) {
    drain(ZSTD_e_continue);
  }
}

void ZstdWriter::close()
{
  if (f == NULL) {
    return;
  }
  drain(ZSTD_e_end);
  ZSTD_freeCCtx(cctx);
  fclose(f);
  cctx = NULL;
  f    = NULL;
}

#endif /* ZSTD_FILE_H */

/* zlib_bench.cpp
 *
 * Copyright 2018 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the Chromium source repository LICENSE file.
 *
 * A benchmark test harness for measuring decoding performance of gzip or zlib
 * (deflate) encoded compressed data. Given a file containing any data, encode
 * (compress) it into gzip or zlib format and then decode (uncompress). Output
 * the median encoding and decoding rates in MB/s.
 *
 * Raw deflate (no gzip or zlib stream wrapper) mode is also supported. Select
 * it with the [raw] argument. Use the [gzip] [zlib] arguments to select those
 * stream wrappers.
 */

#include <algorithm>
#include <string>
#include <vector>

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(_WIN32) && !defined(__clang__)
#define USE_WINDOWS_QPC_CLOCK
#include <windows.h>
#else
#include <sys/time.h>
#endif

extern "C" {
#include "zlib.h"
};

template <size_t B_SIZE_T> class BufferPrintf {
  char buffer_[ B_SIZE_T ];
 public:
  const char* printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    *buffer_ = 0;
    vsnprintf(buffer_, B_SIZE_T, format, args);
    va_end(args);
    return buffer_;
  }
};

void read_file_data_or_exit(const char* file, std::string* data)
{
  FILE* fp = fopen(file, "rb");
  if (!fp) {
    perror(file);
    exit(1);
  }

  data->clear();
  while (!feof(fp)) {
    char buffer[4096];
    size_t size = fread(buffer, 1, 4096, fp);
    if (size || !ferror(fp)) {
      data->append(std::string(buffer, size));
    } else {
      perror("fread");
      exit(1);
    }
  }

  fclose(fp);
}

#if defined(USE_WINDOWS_QPC_CLOCK)

inline double current_time_in_seconds(void) {
  static double clock_frequency;
  static bool have_frequency;

  LARGE_INTEGER qpc;
  ::QueryPerformanceCounter(&qpc);
  if (have_frequency)
    return qpc.QuadPart * clock_frequency;

  have_frequency = true;
  ::QueryPerformanceFrequency(&qpc);
  clock_frequency = 1.0 / (double) qpc.QuadPart;
  return current_time_in_seconds();
}

#else

inline double current_time_in_seconds(void) {
  struct timeval tv;
  gettimeofday(&tv, 0);
  long long micro_seconds = tv.tv_sec * 1000000LL + tv.tv_usec;
  return micro_seconds * 1.0e-6;
}

#endif /* USE_WINDOWS_QPC_CLOCK */

void error_exit(const char* error, int code) {
  fprintf(stderr, "%s (%d)\n", error, code);
  exit(code);
}

inline char* string_as_array(std::string* str) {
  return str->empty() ? nullptr : &*str->begin();
}

/*
 * zlib DEFLATE stream compress / uncompress test harness
 */

bool z_verify_equal(const char* input, size_t size, std::string* output) {
  unsigned char* i = (unsigned char*)input;
  unsigned char* o = (unsigned char*)string_as_array(output);
  if (output->size() == size && !memcmp(o, i, size))
    return true;
  error_exit("fail: decompressed output doesn't match the input data", 2);
  return false;
}

inline int z_compression_bound(int input_data_size) {
  return input_data_size + input_data_size / 1000 + 88;
}

enum z_wrapper {
  kWrapperNONE = 0,
  kWrapperZLIB,
  kWrapperGZIP,
  kWrapperZRAW,
};

inline int z_stream_wrapper_type(const z_wrapper type) {
  if (type == kWrapperZLIB) // zlib DEFLATE stream wrapper
    return MAX_WBITS;
  if (type == kWrapperGZIP) // gzip DEFLATE stream wrapper
    return MAX_WBITS + 16;
  if (type == kWrapperZRAW) // no wrapper, use raw DEFLATE
    return -MAX_WBITS;
  error_exit("unsupported wrapper type", 2);
  return 0;
}

auto z_wrapper_name(const z_wrapper type) {
  if (type == kWrapperZLIB)
    return "ZLIB";
  if (type == kWrapperGZIP)
    return "GZIP";
  if (type == kWrapperZRAW)
    return "RAW";
  error_exit("unsupported wrapper type", 2);
  return "???";
}

bool zlib_compress(
    const z_wrapper type,
    const char* input,
    const size_t input_size,
    std::string* output,
    bool output_size_allocated)
{
  if (!output_size_allocated)
    output->resize(z_compression_bound(input_size));
  uLongf output_size = output->size();

  z_stream stream;
  memset(&stream, 0, sizeof(stream));

  int result = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
      z_stream_wrapper_type(type), MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);

  stream.next_out = (unsigned char*)string_as_array(output);
  stream.avail_out = output_size;
  stream.next_in = (unsigned char*)input;
  stream.avail_in = input_size;

  result |= deflate(&stream, Z_FINISH);
  if (result == Z_STREAM_END)
    output_size = stream.total_out;
  result |= deflateEnd(&stream);
  if (result != Z_STREAM_END)
    error_exit("zlib compress failed", result);

  if (!output_size_allocated)
    output->resize(output_size);
  return true;
}

bool zlib_uncompress(
    const z_wrapper type,
    const std::string& input,
    const size_t output_size,
    std::string* output)
{
  z_stream stream;
  memset(&stream, 0, sizeof(stream));

  int result = inflateInit2(&stream, z_stream_wrapper_type(type));

  stream.next_out = (unsigned char*)string_as_array(output);
  stream.avail_out = output->size();
  stream.next_in = (unsigned char*)input.data();
  stream.avail_in = input.size();

  result |= inflate(&stream, Z_FINISH);
  if (stream.total_out != output_size)
    result = Z_DATA_ERROR, stream.msg = (char*)"decoded data size error";
  result |= inflateEnd(&stream);
  if (result == Z_STREAM_END)
    return true;

  std::string error("zlib uncompress failed: ");
  if (stream.msg)
    error.append(stream.msg);
  error_exit(error.c_str(), result);
  return false;
}

void zlib_file(const char* fname, const z_wrapper type) {
  current_time_in_seconds();

  std::string file;
  read_file_data_or_exit(fname, &file);
  printf("%-40s :\n", fname);

  const int size = file.size();
  const size_t length = size;
  const int repeats = (10485760 + size) / (size + 1);
  const char* data = file.data();

  /*
   * Chop the input data into blocks.
   */
  const int block_size = 1024 << 10;
  const int num_blocks = (length + block_size - 1) / block_size;

  std::vector<const char*> input(num_blocks);
  std::vector<size_t> input_length(num_blocks);
  std::vector<std::string> compressed(num_blocks);
  std::vector<std::string> output(num_blocks);

  for (int b = 0; b < num_blocks; ++b) {
    int input_start = b * block_size;
    int input_limit = std::min<int>((b + 1) * block_size, length);
    input[b] = data + input_start;
    input_length[b] = input_limit - input_start;
    // Pre-grow the output buffer so we don't measure string append time.
    compressed[b].resize(z_compression_bound(block_size));
  }

  /*
   * Run zlib compress/uncompress loop a few times.
   */
  static const int kRuns = 5;
  double ctime[kRuns];
  double utime[kRuns];

  for (int run = 0; run < kRuns; ++run) {
    // Pre-grow the output buffer so we don't measure string append time.
    for (int b = 0; b < num_blocks; ++b)
      compressed[b].resize(z_compression_bound(block_size));

    double deflate_start_time = current_time_in_seconds();
    for (int b = 0; b < num_blocks; ++b)
       for (int i = 0; i < repeats; ++i)
          zlib_compress(type, input[b], input_length[b], &compressed[b], true);
    ctime[run] = current_time_in_seconds() - deflate_start_time;

    // Compress again, resizing compressed, so we don't leave junk at the
    // end of the compressed string that could confuse zlib_uncompress().
    for (int b = 0; b < num_blocks; ++b)
      zlib_compress(type, input[b], input_length[b], &compressed[b], false);

    for (int b = 0; b < num_blocks; ++b)
      output[b].resize(input_length[b]);

    double inflate_start_time = current_time_in_seconds();
    for (int i = 0; i < repeats; ++i)
      for (int b = 0; b < num_blocks; ++b)
        zlib_uncompress(type, compressed[b], input_length[b], &output[b]);
    utime[run] = current_time_in_seconds() - inflate_start_time;

    for (int b = 0; b < num_blocks; ++b)
      z_verify_equal(input[b], input_length[b], &output[b]);
  }

  /*
   * Output the median compress/uncompress run-times.
   */
  int compressed_size = 0;
  for (size_t i = 0; i < compressed.size(); ++i)
    compressed_size += compressed[i].size();

  std::sort(ctime, ctime + kRuns, [](double a, double b) { return a < b; });
  std::sort(utime, utime + kRuns, [](double a, double b) { return a < b; });

  const int median = kRuns / 2;
  float compress_rate = (length / ctime[median]) * repeats / 1048576.0;
  float uncompress_rate = (length / utime[median]) * repeats / 1048576.0;

  std::string urate("???");
  if (uncompress_rate >= 0)
    urate.assign(BufferPrintf<128>().printf("%.1f", uncompress_rate));

  printf("%s: [b %dM] bytes %6d -> %6d %4.1f%% comp %5.1f MB/s "
    "uncomp %5s MB/s\n", z_wrapper_name(type), block_size / (1 << 20),
    static_cast<int>(length), static_cast<uint32_t>(compressed_size),
    (compressed_size * 100.0) / std::max<int>(1, length), compress_rate,
    urate.c_str());
}

int main(int argc, char* argv[])
{
  z_wrapper type = kWrapperNONE;

  if (argc >= 2) {
    if (!strcmp(argv[1], "zlib"))
      type = kWrapperZLIB;
    else if (!strcmp(argv[1], "gzip"))
      type = kWrapperGZIP;
    else if (!strcmp(argv[1], "raw"))
      type = kWrapperZRAW;
  }

  if (type != kWrapperNONE) {
    if (argc >= 3) {
      for (int file = 2; file < argc; ++file)
        zlib_file(argv[file], type);
      return 0;
    }
  }

  printf("usage: %s gzip|zlib|raw files...\n", argv[0]);
  return 1;
}

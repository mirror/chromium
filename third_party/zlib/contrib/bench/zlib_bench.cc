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
 *
 * Note this code can be compiled outside of the chromium build system against
 * the system zlib (-lz) with g++ or clang++ as follows:
 *
 *   g++|clang++ -O3 -Wall -std=c++14 -lstdc++ -lz zlib_bench.cc
 */

#include <algorithm>
#include <string>
#include <vector>

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(_WIN32)
#define USE_WIN32_QPC
#include <windows.h>
#else
#include <sys/time.h>
#endif

extern "C" {
#include "zlib.h"
}

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
  if (data->empty()) {
    fprintf(stderr, "[%s] is empty\n", file);
    exit(1);
  }
}

#if defined(USE_WIN32_QPC)

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
  double seconds = tv.tv_sec + tv.tv_usec * 1.0e-6;
  return seconds;
}

#endif /* USE_WIN32_QPC */

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

inline size_t z_estimate_compressed_size(size_t input_size) {
  return input_size + input_size / 1000 + 88;
}

enum z_wrapper {
  kWrapperNONE,
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
  error_exit("bad wrapper type", int(type));
  return 0;
}

const char* z_wrapper_name(const z_wrapper type) {
  if (type == kWrapperZLIB)
    return "ZLIB";
  if (type == kWrapperGZIP)
    return "GZIP";
  if (type == kWrapperZRAW)
    return "RAW";
  error_exit("bad wrapper type", int(type));
  return "???";
}

void z_compress(
    const z_wrapper type,
    const char* input,
    const size_t input_size,
    std::string* output,
    bool output_size_allocated = true)
{
  if (!output_size_allocated)
    output->resize(z_estimate_compressed_size(input_size));
  auto output_size = output->size();

  z_stream stream;
  memset(&stream, 0, sizeof(stream));

  int result = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
      z_stream_wrapper_type(type), MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);

  stream.next_out = (unsigned char*)string_as_array(output);
  stream.avail_out = (uInt)output_size;
  stream.next_in = (unsigned char*)input;
  stream.avail_in = (uInt)input_size;

  result |= deflate(&stream, Z_FINISH);
  if (result == Z_STREAM_END)
    output_size = stream.total_out;
  result |= deflateEnd(&stream);
  if (result != Z_STREAM_END)
    error_exit("compress failed", result);

  if (!output_size_allocated)
    output->resize(output_size);
}

void z_uncompress(
    const z_wrapper type,
    const std::string& input,
    const size_t output_size,
    std::string* output)
{
  z_stream stream;
  memset(&stream, 0, sizeof(stream));

  int result = inflateInit2(&stream, z_stream_wrapper_type(type));

  stream.next_out = (unsigned char*)string_as_array(output);
  stream.avail_out = (uInt)output->size();
  stream.next_in = (unsigned char*)input.data();
  stream.avail_in = (uInt)input.size();

  result |= inflate(&stream, Z_FINISH);
  if (stream.total_out != output_size)
    result = Z_DATA_ERROR;
  result |= inflateEnd(&stream);
  if (result == Z_STREAM_END)
    return;

  std::string error("uncompress failed: ");
  if (stream.msg)
    error.append(stream.msg);
  error_exit(error.c_str(), result);
}

void z_verify_equal(const char* input, size_t size, std::string* output) {
  unsigned char* in = (unsigned char*)input;
  unsigned char* out = (unsigned char*)string_as_array(output);
  if (output->size() == size && !memcmp(in, out, size))
    return;
  fprintf(stderr, "decompressed data doesn't match the input data\n");
  exit(3);
}

void zlib_file(const char* fname, const z_wrapper type) {
  current_time_in_seconds();

  std::string file;
  read_file_data_or_exit(fname, &file);
  printf("%-40s :\n", fname);

  const int length = static_cast<int>(file.size());
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
    compressed[b].resize(z_estimate_compressed_size(block_size));
  }

  /*
   * Run the zlib compress/uncompress loop a few times.
   */
  const int repeats = (10485760 + length) / (length + 1);
  static const int kRuns = 5;
  double ctime[kRuns];
  double utime[kRuns];

  for (int run = 0; run < kRuns; ++run) {
    // Pre-grow the output buffer so we don't measure string append time.
    for (int b = 0; b < num_blocks; ++b)
      compressed[b].resize(z_estimate_compressed_size(block_size));

    double deflate_start_time = current_time_in_seconds();
    for (int b = 0; b < num_blocks; ++b)
       for (int i = 0; i < repeats; ++i)
          z_compress(type, input[b], input_length[b], &compressed[b]);
    ctime[run] = current_time_in_seconds() - deflate_start_time;

    // Compress again, resizing compressed, so we don't leave junk at the
    // end of the compressed string that could confuse zlib_uncompress().
    for (int b = 0; b < num_blocks; ++b)
      z_compress(type, input[b], input_length[b], &compressed[b], false);

    for (int b = 0; b < num_blocks; ++b)
      output[b].resize(input_length[b]);

    double inflate_start_time = current_time_in_seconds();
    for (int i = 0; i < repeats; ++i)
      for (int b = 0; b < num_blocks; ++b)
        z_uncompress(type, compressed[b], input_length[b], &output[b]);
    utime[run] = current_time_in_seconds() - inflate_start_time;

    for (int b = 0; b < num_blocks; ++b)
      z_verify_equal(input[b], input_length[b], &output[b]);
  }

  /*
   * Output the median compress/uncompress run-times.
   */
  size_t compressed_size = 0;
  for (size_t i = 0; i < compressed.size(); ++i)
    compressed_size += compressed[i].size();

  std::sort(ctime, ctime + kRuns, [](double a, double b) { return a < b; });
  std::sort(utime, utime + kRuns, [](double a, double b) { return a < b; });

  const int median = kRuns / 2;
  double compress_rate = (length / ctime[median]) * repeats / 1048576.0;
  double uncompress_rate = (length / utime[median]) * repeats / 1048576.0;

  std::string urate("???");
  if (uncompress_rate >= 0)
    urate.assign(BufferPrintf<128>().printf("%.1f", uncompress_rate));

  printf("%s: [b %dM] bytes %6d -> %6u %4.1f%% comp %5.1f MB/s "
    "uncomp %5s MB/s\n", z_wrapper_name(type), block_size / (1 << 20),
    static_cast<int>(length), static_cast<uint32_t>(compressed_size),
    (compressed_size * 100.0) / length, compress_rate, urate.c_str());
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

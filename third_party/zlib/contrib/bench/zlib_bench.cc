/* zlib_bench.cc
 *
 * Copyright 2018 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the Chromium source repository LICENSE file.
 *
 * A benchmark test harness for measuring decoding performance of gzip or zlib
 * (deflate) encoded compressed data. Given a file containing any data, encode
 * (compress) it into gzip or zlib format and then decode (uncompress). Output
 * the median and maximum encoding and decoding rates in MB/s.
 *
 * Raw deflate (no gzip or zlib stream wrapper) mode is also supported. Select
 * it with the [raw] argument. Use the [gzip] [zlib] arguments to select those
 * stream wrappers.
 *
 * Note this code can be compiled outside of the Chromium build system against
 * the system zlib (-lz) with g++ or clang++ as follows:
 *
 *   g++|clang++ -O3 -Wall -std=c++14 -lstdc++ -lz zlib_bench.cc
 */

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if !defined(__clang__) && defined(_WIN32)
#define USE_WIN32_QPC
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include "zlib.h"

#if defined(USE_WIN32_QPC)

inline double current_time_in_seconds() {
  static bool have_frequency = false;
  static double frequency;

  LARGE_INTEGER qpc;
  ::QueryPerformanceCounter(&qpc);
  if (have_frequency)
    return qpc.QuadPart * frequency;

  have_frequency = true;
  ::QueryPerformanceFrequency(&qpc);
  frequency = 1.0 / qpc.QuadPart;
  return current_time_in_seconds();
}

#else

inline double current_time_in_seconds() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  double seconds = tv.tv_sec + tv.tv_usec * 1e-6;
  return seconds;
}

#endif /* USE_WIN32_QPC */

void initialize_timer() {
  current_time_in_seconds();
}

void error_exit(const char* error, int code) {
  fprintf(stderr, "%s (%d)\n", error, code);
  exit(code);
}

inline char* string_data(std::string* s) {
  return s->empty() ? nullptr : &*s->begin();
}

void read_file_or_exit(const char* name, std::string* data) {
  const auto mode = std::ios::in | std::ios::binary;

  std::ifstream file(name, mode);
  if (!file) {
    perror(name);
    exit(1);
  }

  file.seekg(0, std::ios::end);
  data->resize(file.tellg());
  file.seekg(0, std::ios::beg);
  file.read(string_data(data), data->size());

  if (file.bad()) {
    perror((std::string("error reading ") + name).c_str());
    exit(1);
  } else if (data->empty()) {
    fprintf(stderr, "%s is empty\n", name);
    exit(1);
  }
}

inline std::string string_number(double n) {
  char number[32];
  sprintf(number, "%5.1f", n);
  return number;
}

/*
 * zlib DEFLATE stream compress / uncompress test harness.
 */

inline size_t z_estimate_compressed_size(size_t input_size) {
  return compressBound(input_size);
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
  return nullptr;
}

void z_compress(
    const z_wrapper type,
    const char* input,
    const size_t input_size,
    std::string* output,
    bool resize_output = false)
{
  if (resize_output)
    output->resize(z_estimate_compressed_size(input_size));
  size_t output_size = output->size();

  z_stream stream;
  memset(&stream, 0, sizeof(stream));

  int result = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
      z_stream_wrapper_type(type), MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
  if (result != Z_OK)
    error_exit("deflateInit2 failed", result);

  stream.next_out = (Bytef*)string_data(output);
  stream.avail_out = (uInt)output_size;
  stream.next_in = (z_const Bytef*)input;
  stream.avail_in = (uInt)input_size;

  result = deflate(&stream, Z_FINISH);
  if (result == Z_STREAM_END)
    output_size = stream.total_out;
  result |= deflateEnd(&stream);
  if (result != Z_STREAM_END)
    error_exit("compress failed", result);

  if (resize_output)
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
  if (result != Z_OK)
    error_exit("inflateInit2 failed", result);

  stream.next_out = (Bytef*)string_data(output);
  stream.avail_out = (uInt)output->size();
  stream.next_in = (z_const Bytef*)input.data();
  stream.avail_in = (uInt)input.size();

  result = inflate(&stream, Z_FINISH);
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

void verify_equal(const char* input, size_t size, std::string* output) {
  const auto* data = string_data(output);
  if (output->size() == size && !memcmp(data, input, size))
    return;
  fprintf(stderr, "uncompressed data does not match the input data\n");
  exit(3);
}

void zlib_file(const char* name, const z_wrapper type) {
  initialize_timer();

  std::string file;
  read_file_or_exit(name, &file);
  printf("%-40s :\n", name);

  const int length = static_cast<int>(file.size());
  const char* data = file.data();

  /*
   * Chop the input data into blocks.
   */
  const int block_size = 1 << 20;
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
   * Run the zlib compress/uncompress loop a few times with |repeats| to
   * process about 10MB of input data per run.
   */
  const int kMegaByte = 1024 * 1024;
  const int repeats = (10 * kMegaByte + length) / (length + 1);
  const int kRuns = 5;
  double ctime[kRuns];
  double utime[kRuns];

  for (int run = 0; run < kRuns; ++run) {
    // Pre-grow the output buffer so we don't measure string append time.
    for (int b = 0; b < num_blocks; ++b)
      compressed[b].resize(z_estimate_compressed_size(block_size));

    double deflate_start_time = current_time_in_seconds();
    for (int r = 0; r < repeats; ++r)
      for (int b = 0; b < num_blocks; ++b)
        z_compress(type, input[b], input_length[b], &compressed[b]);
    ctime[run] = current_time_in_seconds() - deflate_start_time;

    // Compress again, resizing compressed, so we don't leave junk at the
    // end of the compressed string that could confuse z_uncompress().
    for (int b = 0; b < num_blocks; ++b)
      z_compress(type, input[b], input_length[b], &compressed[b], true);

    for (int b = 0; b < num_blocks; ++b)
      output[b].resize(input_length[b]);

    double inflate_start_time = current_time_in_seconds();
    for (int r = 0; r < repeats; ++r)
      for (int b = 0; b < num_blocks; ++b)
        z_uncompress(type, compressed[b], input_length[b], &output[b]);
    utime[run] = current_time_in_seconds() - inflate_start_time;

    for (int b = 0; b < num_blocks; ++b)
      verify_equal(input[b], input_length[b], &output[b]);
  }

  /*
   * Output the median/maximum compress/uncompress rates in MB/s.
   */
  size_t output_length = 0;
  for (size_t i = 0; i < compressed.size(); ++i)
    output_length += compressed[i].size();

  std::sort(ctime, ctime + kRuns);
  std::sort(utime, utime + kRuns);

  int median = kRuns / 2;
  double deflate_rate = length * repeats / kMegaByte / ctime[median];
  double inflate_rate = length * repeats / kMegaByte / utime[median];

  std::string irate("???");
  if (inflate_rate >= 0)
    irate = string_number(inflate_rate);

  std::string drate("???");
  if (deflate_rate >= 0)
    drate = string_number(deflate_rate);

  double deflate_rate_max = length * repeats / kMegaByte / ctime[0];
  double inflate_rate_max = length * repeats / kMegaByte / utime[0];

  std::string irate_max("???");
  if (inflate_rate_max >= 0)
    irate_max = string_number(inflate_rate_max);

  std::string drate_max("???");
  if (deflate_rate_max >= 0)
    drate_max = string_number(deflate_rate_max);

  // type, block size, compression ratio, etc
  printf("%s: [b %dM] bytes %6d -> %6u %4.1f%%",
    z_wrapper_name(type), block_size / (1 << 20), static_cast<int>(length),
    static_cast<unsigned>(output_length), output_length * 100.0 / length);

  // compress / uncompress median (max) rates
  printf(" comp %s (%s) MB/s uncomp %s (%s) MB/s\n",
    drate.c_str(), drate_max.c_str(), irate.c_str(), irate_max.c_str());
}

int main(int argc, char* argv[]) {
  z_wrapper type = kWrapperNONE;

  if (argc > 1) {
    if (!strcmp(argv[1], "zlib"))
      type = kWrapperZLIB;
    else if (!strcmp(argv[1], "gzip"))
      type = kWrapperGZIP;
    else if (!strcmp(argv[1], "raw"))
      type = kWrapperZRAW;
  }

  if ((type != kWrapperNONE) && (argc > 2)) {
    for (int file = 2; file < argc; ++file)
      zlib_file(argv[file], type);
    return 0;
  }

  printf("usage: %s gzip|zlib|raw files...\n", argv[0]);
  return 1;
}

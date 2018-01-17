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
 *   g++|clang++ -O3 -Wall -std=c++98 -lstdc++ -lz zlib_bench.cc
 */

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if !defined(__clang__) && defined(_WIN32)
#define WIN32_QPC_TIME
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include "zlib.h"

void error_exit(const char* error, int code) {
  fprintf(stderr, "%s (%d)\n", error, code);
  exit(code);
}

inline char* string_data(std::string* s) {
  if (s->empty())
    error_exit("failure: empty string data", 3);
  return &*s->begin();
}

void read_file_or_exit(const char* name, std::string* data) {
  std::ifstream file;

  file.open(name, std::ios::in | std::ios::binary);
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

#if defined(WIN32_QPC_TIME)

inline double current_time_in_seconds() {
  LARGE_INTEGER qpf;
  ::QueryPerformanceFrequency(&qpf);

  LARGE_INTEGER qpc;
  ::QueryPerformanceCounter(&qpc);
  return qpc.QuadPart * 1.0 / qpf.QuadPart;
}

#else

inline double current_time_in_seconds() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  double seconds = tv.tv_sec + tv.tv_usec * 1e-6;
  return seconds;
}

#endif /* WIN32_QPC_TIME */

/*
 * zlib DEFLATE stream compress / uncompress test harness.
 */

inline size_t zlib_estimate_compressed_size(size_t input_size) {
  return compressBound(input_size);
}

enum z_wrapper {
  kWrapperNONE,
  kWrapperZLIB,
  kWrapperGZIP,
  kWrapperZRAW,
};

inline int zlib_stream_wrapper_type(const z_wrapper type) {
  if (type == kWrapperZLIB) // zlib DEFLATE stream wrapper
    return MAX_WBITS;
  if (type == kWrapperGZIP) // gzip DEFLATE stream wrapper
    return MAX_WBITS + 16;
  if (type == kWrapperZRAW) // no wrapper, use raw DEFLATE
    return -MAX_WBITS;
  error_exit("bad wrapper type", int(type));
  return 0;
}

const char* zlib_wrapper_name(const z_wrapper type) {
  if (type == kWrapperZLIB)
    return "ZLIB";
  if (type == kWrapperGZIP)
    return "GZIP";
  if (type == kWrapperZRAW)
    return "RAW";
  error_exit("bad wrapper type", int(type));
  return 0;
}

void zlib_compress(
    const z_wrapper type,
    const char* input,
    const size_t input_size,
    std::string* output,
    bool resize_output = false)
{
  if (resize_output)
    output->resize(zlib_estimate_compressed_size(input_size));
  size_t output_size = output->size();

  z_stream stream;
  memset(&stream, 0, sizeof(stream));

  int result = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
      zlib_stream_wrapper_type(type), MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
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

void zlib_uncompress(
    const z_wrapper type,
    const std::string& input,
    const size_t output_size,
    std::string* output)
{
  z_stream stream;
  memset(&stream, 0, sizeof(stream));

  int result = inflateInit2(&stream, zlib_stream_wrapper_type(type));
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
  const char* data = string_data(output);
  if (output->size() == size && !memcmp(data, input, size))
    return;
  fprintf(stderr, "uncompressed data does not match the input data\n");
  exit(3);
}

void zlib_file(const char* name, const z_wrapper type) {
  /*
   * Read the input file data.
   */
  std::string file;
  read_file_or_exit(name, &file);
  printf("%-40s :\n", name);

  const char* data = file.data();
  const int length = static_cast<int>(file.size());

  /*
   * Chop the data into blocks.
   */
  const int block_size = 1 << 20;
  const int blocks = (length + block_size - 1) / block_size;

  std::vector<const char*> input(blocks);
  std::vector<size_t> input_length(blocks);
  std::vector<std::string> compressed(blocks);
  std::vector<std::string> output(blocks);

  for (int b = 0; b < blocks; ++b) {
    int input_start = b * block_size;
    int input_limit = std::min<int>((b + 1) * block_size, length);
    input[b] = data + input_start;
    input_length[b] = input_limit - input_start;
  }

  /*
   * Run the zlib compress/uncompress loop a few times.
   */
  const int mega_byte = 1024 * 1024;
  const int repeats = (10 * mega_byte + length) / (length + 1);
  const int runs = 5;
  double ctime[runs];
  double utime[runs];

  for (int run = 0; run < runs; ++run) {
    // Pre-grow the output buffer so we don't measure string append time.
    for (int b = 0; b < blocks; ++b)
      compressed[b].resize(zlib_estimate_compressed_size(block_size));

    double deflate_start_time = current_time_in_seconds();
    for (int r = 0; r < repeats; ++r)
      for (int b = 0; b < blocks; ++b)
        zlib_compress(type, input[b], input_length[b], &compressed[b]);
    ctime[run] = current_time_in_seconds() - deflate_start_time;

    // Compress again, resizing compressed, so we don't leave junk at the
    // end of the compressed string that could confuse zlib_uncompress().
    for (int b = 0; b < blocks; ++b)
      zlib_compress(type, input[b], input_length[b], &compressed[b], true);

    for (int b = 0; b < blocks; ++b)
      output[b].resize(input_length[b]);

    double inflate_start_time = current_time_in_seconds();
    for (int r = 0; r < repeats; ++r)
      for (int b = 0; b < blocks; ++b)
        zlib_uncompress(type, compressed[b], input_length[b], &output[b]);
    utime[run] = current_time_in_seconds() - inflate_start_time;

    for (int b = 0; b < blocks; ++b)
      verify_equal(input[b], input_length[b], &output[b]);
  }

  /*
   * Output the median/maximum compress/uncompress rates in MB/s.
   */
  size_t output_length = 0;
  for (size_t i = 0; i < compressed.size(); ++i)
    output_length += compressed[i].size();

  std::sort(ctime, ctime + runs);
  std::sort(utime, utime + runs);

  int median = runs / 2;
  double deflate_rate = length * repeats / mega_byte / ctime[median];
  double inflate_rate = length * repeats / mega_byte / utime[median];

  std::string drate("???");
  if (deflate_rate >= 0)
    drate = string_number(deflate_rate);

  std::string irate("???");
  if (inflate_rate >= 0)
    irate = string_number(inflate_rate);

  double deflate_rate_max = length * repeats / mega_byte / ctime[0];
  double inflate_rate_max = length * repeats / mega_byte / utime[0];

  std::string drate_max("???");
  if (deflate_rate_max >= 0)
    drate_max = string_number(deflate_rate_max);

  std::string irate_max("???");
  if (inflate_rate_max >= 0)
    irate_max = string_number(inflate_rate_max);

  // type, block size, compression ratio, etc
  printf("%s: [b %dM] bytes %6d -> %6u %4.1f%%",
    zlib_wrapper_name(type), block_size / (1 << 20), static_cast<int>(length),
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

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Provides a minimal wrapping of the Blink image decoders. Used to perform
// a non-threaded, memory-to-memory image decode using micro second accuracy
// clocks to measure image decode time. Optionally applies color correction
// during image decoding on supported platforms (default off). Usage:
//
//   % ninja -C out/Release image_decode_bench &&
//      ./out/Release/image_decode_bench file [iterations]
//
// TODO(noel): Consider adding md5 checksum support to WTF. Use it to compute
// the decoded image frame md5 and output that value.
//
// TODO(noel): Consider integrating this tool in Chrome telemetry for realz,
// using the image corpora used to assess Blink image decode performance. See
// http://crbug.com/398235#c103 and http://crbug.com/258324#c5

#include <fstream>

#include "base/command_line.h"
#include "base/memory/scoped_refptr.h"
#include "base/message_loop/message_loop.h"
#include "mojo/edk/embedder/embedder.h"
#include "platform/SharedBuffer.h"
#include "platform/graphics/ImageDataBuffer.h"
#include "platform/image-decoders/ImageDecoder.h"
#include "platform/image-encoders/ImageEncoderUtils.h"
#include "platform/network/mime/MIMETypeRegistry.h"
#include "platform/wtf/Time.h"
#include "platform/wtf/Vector.h"
#include "public/platform/Platform.h"

namespace blink {

namespace {

scoped_refptr<SharedBuffer> ReadFile(const char* name) {
  std::ifstream file(name, std::ios::in | std::ios::binary);
  if (!file) {
    fprintf(stderr, "Cannot open %s\n", name);
    exit(2);
  }

  file.seekg(0, std::ios::end);
  size_t file_size = file.tellg();
  file.seekg(0, std::ios::beg);

  if (!file || !file_size)
    return SharedBuffer::Create();

  Vector<char> buffer(file_size);
  if (!file.read(buffer.data(), file_size)) {
    fprintf(stderr, "Error reading %s\n", name);
    exit(2);
  }

  return SharedBuffer::AdoptVector(buffer);
}

struct ImageMeta {
  char* name;
  int width;
  int height;
  int frames;
  double time;
};

void DecodeFailure(ImageMeta* image) {
  fprintf(stderr, "Failed to decode image %s\n", image->name);
  exit(3);
}

void DecodeImageData(SharedBuffer* data, size_t packet_size, ImageMeta* image) {
  std::unique_ptr<ImageDecoder> decoder = ImageDecoder::Create(
      data, true, ImageDecoder::kAlphaPremultiplied, ColorBehavior::Ignore());

  if (!packet_size) {
    auto start = CurrentTimeTicks();

    bool all_data_received = true;
    decoder->SetData(data, all_data_received);

    int frame_count = decoder->FrameCount();
    for (int index = 0; index < frame_count; ++index) {
      if (!decoder->DecodeFrameBufferAtIndex(index))
        DecodeFailure(image);
    }

    image->time += (CurrentTimeTicks() - start).InSecondsF();
    image->width = decoder->Size().Width();
    image->height = decoder->Size().Height();
    image->frames = frame_count;

    if (!frame_count || decoder->Failed())
      DecodeFailure(image);
    return;
  }

  scoped_refptr<SharedBuffer> packet_data = SharedBuffer::Create();
  size_t position = 0;
  int frame_count = 0;
  int index = 0;

  auto start = CurrentTimeTicks();

  while (true) {
    const char* packet;
    size_t length = data->GetSomeData(packet, position);

    length = std::min(length, packet_size);
    packet_data->Append(packet, length);
    position += length;

    bool all_data_received = position >= data->size();
    decoder->SetData(packet_data.get(), all_data_received);

    frame_count = decoder->FrameCount();
    for (; index < frame_count; ++index) {
      ImageFrame* frame = decoder->DecodeFrameBufferAtIndex(index);
      if (frame->GetStatus() != ImageFrame::kFrameComplete)
        DecodeFailure(image);
    }

    if (all_data_received || decoder->Failed())
      break;
  }

  image->time += (CurrentTimeTicks() - start).InSecondsF();
  image->width = decoder->Size().Width();
  image->height = decoder->Size().Height();
  image->frames = frame_count;

  if (!frame_count || decoder->Failed())
    DecodeFailure(image);
}

bool DecodeBench(SharedBuffer* data, ImageMeta* image) {
  const int kRuns = 11;

  const int kArea = image->width * image->height;
  int repeats = (18 * 1024 * 1024) / kArea;
  if (repeats < 10)
    repeats = 10;
  if (repeats > 10000)
    repeats = 10000;

  double utime[kRuns];
  for (int run = 0; run < kRuns; ++run) {
    image->time = 0.0;
    for (int i = 0; i < repeats; ++i)
      DecodeImageData(data, 0, image);
    utime[run] = image->time;
  }

  std::sort(utime, utime + kRuns);
  const int median = kRuns / 2;

  float m_rate = 4 * kArea * repeats / utime[median];
  m_rate /= 1024.0 * 1024.0;  // rate in MB/s
  float s_rate = 4 * kArea * repeats / utime[0];
  s_rate /= 1024.0 * 1024.0;  // rate in MB/s

  printf("%.6f %.1f ", utime[0], s_rate);
  printf("%.6f %.1f ", utime[median], m_rate);
  printf("\n");

  return false;
}

void EncodeImageData(const String& mime_type,
                     ImageDataBuffer* buffer,
                     ImageMeta* image) {
  Vector<unsigned char> encoded_image;

  auto start = CurrentTimeTicks();
  bool success = buffer->EncodeImage(mime_type, 0.8, &encoded_image);
  image->time += (CurrentTimeTicks() - start).InSecondsF();

  if (!success) {
    fprintf(stderr, "Failed to encode image %s\n", image->name);
    exit(3);
  }
}

String EncodingMimeType(const String& extension) {
  String mime_type =
      MIMETypeRegistry::GetWellKnownMIMETypeForExtension(extension);
  return ImageEncoderUtils::ToEncodingMimeType(
      mime_type, ImageEncoderUtils::kEncodeReasonToDataURL);
}

bool EncodeBench(SharedBuffer* data, ImageMeta* image) {
  const int kRuns = 11;

  std::unique_ptr<ImageDecoder> decoder = ImageDecoder::Create(
      data, true, ImageDecoder::kAlphaPremultiplied, ColorBehavior::Ignore());

  bool frame_complete = true;
  ImageFrame* frame = decoder->DecodeFrameBufferAtIndex(0);
  if (frame->GetStatus() != ImageFrame::kFrameComplete)
    frame_complete = false;
  if (decoder->FilenameExtension().IsEmpty())
    frame_complete = false;
  int frame_count = decoder->FrameCount();
  if (!frame_count || !frame_complete || decoder->Failed())
    DecodeFailure(image);

  image->width = decoder->Size().Width();
  image->height = decoder->Size().Height();
  image->frames = frame_count;

  const int kArea = image->width * image->height;
  int repeats = (18 * 1024 * 1024) / kArea;
  if (repeats < 10)
    repeats = 10;
  if (repeats > 10000)
    repeats = 10000;

  const String kMimeType = EncodingMimeType(decoder->FilenameExtension());
  unsigned char const* kPixels = (unsigned char*)frame->GetAddr(0, 0);
  const IntSize kSize = IntSize(image->width, image->height);
  ImageDataBuffer buffer = ImageDataBuffer(kSize, kPixels);

  double utime[kRuns];
  for (int run = 0; run < kRuns; ++run) {
    image->time = 0.0;
    for (int i = 0; i < repeats; ++i)
      EncodeImageData(kMimeType, &buffer, image);
    utime[run] = image->time;
  }

  std::sort(utime, utime + kRuns);
  const int median = kRuns / 2;

  float m_rate = 4 * kArea * repeats / utime[median];
  m_rate /= 1024.0 * 1024.0;  // rate in MB/s
  float s_rate = 4 * kArea * repeats / utime[0];
  s_rate /= 1024.0 * 1024.0;  // rate in MB/s

  printf("%.6f %.1f ", utime[0], s_rate);
  printf("%.6f %.1f ", utime[median], m_rate);
  printf("\n");

  return false;
}

}  // namespace

int Main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  const char* name = argv[0];

  bool encode_bench = false;
  if (argc >= 2 && !strcmp(argv[1], "--encode"))
    encode_bench = (--argc, ++argv, true);

  bool decode_bench = false;
  if (argc >= 2 && !strcmp(argv[1], "--decode"))
    decode_bench = (--argc, ++argv, true);

  if (argc < 2) {
    fprintf(stderr,
            "Usage: %s [--encode|decode] file [iterations] [packetSize]\n",
            name);
    exit(1);
  }

  // Control decode iterations and packet size.

  size_t decode_iterations = 1;
  if (!encode_bench && !decode_bench && argc >= 3) {
    char* end = nullptr;
    decode_iterations = strtol(argv[2], &end, 10);
    if (*end != '\0' || !decode_iterations) {
      fprintf(stderr,
              "Second argument should be number of iterations. "
              "The default is 1. You supplied %s\n",
              argv[2]);
      exit(1);
    }
  }

  size_t packet_size = 0;
  if (!encode_bench && !decode_bench && argc >= 4) {
    char* end = nullptr;
    packet_size = strtol(argv[3], &end, 10);
    if (*end != '\0') {
      fprintf(stderr,
              "Third argument should be packet size. Default is "
              "0, meaning to decode the entire image in one packet. You "
              "supplied %s\n",
              argv[3]);
      exit(1);
    }
  }

  // Create a web platform.  blink::Platform can't be used directly because
  // it has a protected constructor.

  class WebPlatform : public Platform {};
  Platform::Initialize(new WebPlatform());

  // Read entire file content to data and consolidate the SharedBuffer data
  // segments into one, contiguous block of memory.

  scoped_refptr<SharedBuffer> data = ReadFile(argv[1]);
  if (!data.get() || !data->size()) {
    fprintf(stderr, "Error reading image %s\n", argv[1]);
    exit(2);
  }

  data->Data();

  // Warm-up: throw out the first iteration for more consistent results.

  ImageMeta image;
  image.name = argv[1];
  DecodeImageData(data.get(), packet_size, &image);

  // Run image encode or decode rate-based bench.

  if (decode_bench)
    return DecodeBench(data.get(), &image);
  if (encode_bench)
    return EncodeBench(data.get(), &image);

  // Otherwise, decode bench for decode iterations.

  double total_time = 0.0;
  for (size_t i = 0; i < decode_iterations; ++i) {
    image.time = 0.0;
    DecodeImageData(data.get(), packet_size, &image);
    total_time += image.time;
  }

  // Results to stdout.

  double average_time = total_time / decode_iterations;
  printf("%f %f\n", total_time, average_time);
  return 0;
}

}  // namespace blink

int main(int argc, char* argv[]) {
  base::MessageLoop message_loop;
  mojo::edk::Init();
  return blink::Main(argc, argv);
}

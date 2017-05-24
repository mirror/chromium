// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/image-encoders/ImageEncoder.h"

namespace blink {

bool ImageEncoder::Encode(Vector<unsigned char>* dst,
                          const SkPixmap& src,
                          const SkJpegEncoder::Options& options) {
  VectorWStream dst_stream(dst);
  return SkJpegEncoder::Encode(&dst_stream, src, options);
}

bool ImageEncoder::Encode(Vector<unsigned char>* dst,
                          const SkPixmap& src,
                          const SkPngEncoder::Options& options) {
  VectorWStream dst_stream(dst);
  return SkPngEncoder::Encode(&dst_stream, src, options);
}

bool ImageEncoder::Encode(Vector<unsigned char>* dst,
                          const SkPixmap& src,
                          const SkWebpEncoder::Options& options) {
  VectorWStream dst_stream(dst);
  return SkWebpEncoder::Encode(&dst_stream, src, options);
}

std::unique_ptr<ImageEncoder> ImageEncoder::Create(Vector<unsigned char>* dst,
                                                   const SkPixmap& src,
                                                   const MimeType type,
                                                   const double quality) {
  // TODO(Savago): When we have C++14, use std::make_unique.
  auto encoder = std::unique_ptr<ImageEncoder>(new ImageEncoder(dst));
  switch (type) {
    case kMimeTypeJpeg: {
      SkJpegEncoder::Options jpgOptions;
      jpgOptions.fQuality = ImageEncoder::ComputeJpegQuality(quality);
      jpgOptions.fAlphaOption = SkJpegEncoder::AlphaOption::kBlendOnBlack;
      jpgOptions.fBlendBehavior = SkTransferFunctionBehavior::kIgnore;
      if (jpgOptions.fQuality == 100) {
        jpgOptions.fDownsample = SkJpegEncoder::Downsample::k444;
      }
      encoder->encoder_ = SkJpegEncoder::Make(&encoder->dst_, src, jpgOptions);
      break;
    }
    case kMimeTypePng: {
      // This parameter is only used for JPEG.
      DCHECK(quality == 1.0);
      SkPngEncoder::Options pngOptions;
      pngOptions.fFilterFlags = SkPngEncoder::FilterFlag::kSub;
      pngOptions.fZLibLevel = 3;
      pngOptions.fUnpremulBehavior = SkTransferFunctionBehavior::kIgnore;
      encoder->encoder_ = SkPngEncoder::Make(&encoder->dst_, src, pngOptions);
      break;
    }
    // TODO(Savago): should we have a factory for WebP encoder?
    default:
      encoder->encoder_ = nullptr;
      break;
  }

  if (!encoder->encoder_) {
    return nullptr;
  }
  return encoder;
}

int ImageEncoder::ComputeJpegQuality(double quality) {
  int compression_quality = 92;  // Default value
  if (0.0f <= quality && quality <= 1.0)
    compression_quality = static_cast<int>(quality * 100 + 0.5);
  return compression_quality;
}

SkWebpEncoder::Options ImageEncoder::ComputeWebpOptions(
    double quality,
    SkTransferFunctionBehavior unpremulBehavior) {
  SkWebpEncoder::Options options;
  options.fUnpremulBehavior = unpremulBehavior;

  if (quality == 1.0) {
    // Choose a lossless encode.  When performing a lossless encode, higher
    // quality corresponds to slower encoding and smaller output size.
    options.fCompression = SkWebpEncoder::Compression::kLossless;
    options.fQuality = 75.0f;
  } else {
    options.fQuality = 80.0f;  // Default value
    if (0.0f <= quality && quality <= 1.0)
      options.fQuality = quality * 100.0f;
  }

  return options;
}
};

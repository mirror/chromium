// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/image_provider.h"

namespace cc {

ImageProvider::ScopedDecodedDrawImage::ScopedDecodedDrawImage() = default;

ImageProvider::ScopedDecodedDrawImage::ScopedDecodedDrawImage(
    DecodedDrawImage image)
    : image_(std::move(image)) {}

ImageProvider::ScopedDecodedDrawImage::ScopedDecodedDrawImage(
    DecodedDrawImage image,
    DestructionCallback callback)
    : image_(std::move(image)), destruction_callback_(std::move(callback)) {}

ImageProvider::ScopedDecodedDrawImage::ScopedDecodedDrawImage(
    ScopedDecodedDrawImage&& other) {
  image_ = std::move(other.image_);
  destruction_callback_ = std::move(other.destruction_callback_);
}

ImageProvider::ScopedDecodedDrawImage& ImageProvider::ScopedDecodedDrawImage::
operator=(ScopedDecodedDrawImage&& other) {
  DestroyDecode();

  image_ = std::move(other.image_);
  destruction_callback_ = std::move(other.destruction_callback_);
  return *this;
}

ImageProvider::ScopedDecodedDrawImage::~ScopedDecodedDrawImage() {
  DestroyDecode();
}

void ImageProvider::ScopedDecodedDrawImage::DestroyDecode() {
  if (!destruction_callback_.is_null())
    std::move(destruction_callback_).Run(std::move(image_));
}

ImageProvider::ScopedImageDecoder::ScopedImageDecoder(
    ImageProvider* provider,
    const std::vector<DrawImage>& images) {
  DCHECK(provider || images.empty());
  for (auto& draw_image : images)
    decoded_.push_back(provider->GetDecodedDrawImage(draw_image));
}

ImageProvider::ScopedImageDecoder::~ScopedImageDecoder() = default;

}  // namespace cc

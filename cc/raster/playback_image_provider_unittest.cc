// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/raster/playback_image_provider.h"

#include "cc/paint/paint_image_builder.h"
#include "cc/test/skia_common.h"
#include "cc/test/stub_decode_cache.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

sk_sp<SkImage> CreateRasterImage() {
  SkBitmap bitmap;
  bitmap.allocN32Pixels(10, 10);
  return SkImage::MakeFromBitmap(bitmap);
}

DecodedDrawImage CreateDecode() {
  return DecodedDrawImage(CreateRasterImage(), SkSize::MakeEmpty(),
                          SkSize::Make(1.0f, 1.0f), kMedium_SkFilterQuality);
}

class MockDecodeCache : public StubDecodeCache {
 public:
  MockDecodeCache() = default;
  ~MockDecodeCache() override { EXPECT_EQ(refed_image_count_, 0); }

  DecodedDrawImage GetDecodedImageForDraw(
      const DrawImage& draw_image) override {
    last_image_ = draw_image;
    images_decoded_++;
    refed_image_count_++;
    return CreateDecode();
  }

  void DrawWithImageFinished(
      const DrawImage& draw_image,
      const DecodedDrawImage& decoded_draw_image) override {
    refed_image_count_--;
    EXPECT_GE(refed_image_count_, 0);
  }

  int refed_image_count() const { return refed_image_count_; }
  int images_decoded() const { return images_decoded_; }
  const DrawImage& last_image() { return last_image_; }

 private:
  int refed_image_count_ = 0;
  int images_decoded_ = 0;
  DrawImage last_image_;
};

}  // namespace
}  // namespace cc

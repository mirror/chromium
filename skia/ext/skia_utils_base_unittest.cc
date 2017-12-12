// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "skia/ext/skia_utils_base.h"

#include "base/memory/ptr_util.h"
#include "base/test/gtest_util.h"
#include "base/test/test_discardable_memory_allocator.h"
#include "skia/ext/skia_utils_base.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSerialProcs.h"
#include "third_party/skia/include/effects/SkImageSource.h"
#include "ui/gfx/codec/png_codec.h"

namespace skia {
namespace {

TEST(SkiaUtilsBaseTest, DeserializationWithEncodedImages) {
  base::TestDiscardableMemoryAllocator allocator;
  base::DiscardableMemoryAllocator::SetInstance(&allocator);

  SkPMColor pixel = 0xFFFFFFFF;  // white;
  SkImageInfo info = SkImageInfo::MakeN32Premul(1, 1);
  SkPixmap pm = {info, &pixel, sizeof(SkPMColor)};
  auto image = SkImage::MakeRasterCopy(pm);
  auto jpeg_data = image->encodeToData(SkEncodedImageFormat::kJPEG, 100);
  auto filter = SkImageSource::Make(SkImage::MakeFromEncoded(jpeg_data));

  sk_sp<SkData> data(ValidatingSerializeFlattenable(filter.get()));

  // Now we install a proc to see that all embedded images have been converted
  // to png.
  SkDeserialProcs procs;
  procs.fImageProc = [](const void* data, size_t length,
                        void*) -> sk_sp<SkImage> {
    SkBitmap bitmap;
    EXPECT_TRUE(gfx::PNGCodec::Decode(static_cast<const uint8_t*>(data), length,
                                      &bitmap));
    return nullptr;  // allow for normal deserialization
  };
  SkFlattenable::Deserialize(SkImageFilter::GetFlattenableType(), data->data(),
                             data->size(), &procs);

  base::DiscardableMemoryAllocator::SetInstance(nullptr);
}

}  // namespace
}  // namespace skia

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/test/test_discardable_memory_allocator.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkFlattenableSerialization.h"
#include "third_party/skia/include/core/SkImageFilter.h"

static const int kBitmapSize = 24;

/* Environment for the fuzzer. */
struct Environment {
  Environment() {
    base::DiscardableMemoryAllocator::SetInstance(
        &discardable_memory_allocator);
  }

  base::TestDiscardableMemoryAllocator discardable_memory_allocator;
};

// Entry point for LibFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  static Environment* env = new Environment();
  ALLOW_UNUSED_LOCAL(env);

  SkBitmap bitmap;
  bitmap.allocN32Pixels(kBitmapSize, kBitmapSize);
  SkCanvas canvas(bitmap);
  canvas.clear(0x00000000);

  // This call shouldn't crash or cause ASAN to flag any memory issues
  // If nothing bad happens within this call, everything is fine
  sk_sp<SkImageFilter> flattenable =
      SkValidatingDeserializeImageFilter(data, size);

  // Adding some info, but the test passed if we got here without any trouble
  if (flattenable) {
    // Let's see if using the filters can cause any trouble...
    SkPaint paint;
    paint.setImageFilter(flattenable);
    canvas.clipRect(SkRect::MakeXYWH(0, 0, SkIntToScalar(kBitmapSize),
                                     SkIntToScalar(kBitmapSize)));

    // This call shouldn't crash or cause ASAN to flag any memory issues
    // If nothing bad happens within this call, everything is fine
    canvas.drawBitmap(bitmap, 0, 0, &paint);
  }
  return 0;
}

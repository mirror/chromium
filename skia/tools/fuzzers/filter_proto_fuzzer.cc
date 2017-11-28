// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Takes an ImageFilterChild protobuf message from libprotobuf-mutator, converts
// it to an actual skia image filter and uses it to fuzz skia.

#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <string>

#include "skia/tools/fuzzers/filter_proto_converter.h"

#include "base/process/memory.h"
#include "base/test/test_discardable_memory_allocator.h"
#include "third_party/libprotobuf-mutator/src/src/libfuzzer/libfuzzer_macro.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkFlattenableSerialization.h"
#include "third_party/skia/include/core/SkImageFilter.h"

protobuf_mutator::protobuf::LogSilencer log_silencer;

using filter_proto_converter::Input;
using filter_proto_converter::Converter;

static const int kBitmapSize = 24;

struct Environment {
  base::TestDiscardableMemoryAllocator* discardable_memory_allocator;
  Environment() {
    base::EnableTerminationOnOutOfMemory();
    discardable_memory_allocator = new base::TestDiscardableMemoryAllocator();
    base::DiscardableMemoryAllocator::SetInstance(discardable_memory_allocator);
  }
};

DEFINE_PROTO_FUZZER(const Input& input) {
  static Environment environment = Environment();
  ALLOW_UNUSED_LOCAL(environment);

  static Converter converter = Converter();

  SkBitmap bitmap;
  bitmap.allocN32Pixels(kBitmapSize, kBitmapSize);
  SkCanvas canvas(bitmap);
  canvas.clear(0x00000000);

  std::string ipc_filter_message = converter.Convert(input);

  // Allow the flattened skia filter to be retrieved easily.
  if (getenv("LPM_DUMP_NATIVE_INPUT")) {
    std::cout << ipc_filter_message;
    fflush(0);
  }

  sk_sp<SkImageFilter> flattenable = SkValidatingDeserializeImageFilter(
      ipc_filter_message.c_str(), ipc_filter_message.size());

  // !!! CHECK flattenable.
  if (flattenable) {
    SkPaint paint;
    paint.setImageFilter(flattenable);
    canvas.save();

    canvas.clipRect(SkRect::MakeXYWH(0, 0, SkIntToScalar(kBitmapSize),
                                     SkIntToScalar(kBitmapSize)));
    canvas.drawBitmap(bitmap, 0, 0, &paint);
    canvas.restore();
  }
}

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKIA_PUBLIC_INTERFACES_COLOR_STRUCT_TRAITS_H_
#define SKIA_PUBLIC_INTERFACES_COLOR_STRUCT_TRAITS_H_

#include "skia/public/interfaces/color.mojom.h"
#include "third_party/skia/include/core/SkColor.h"

namespace mojo {

template <>
struct StructTraits<skia::mojom::ColorDataView, SkColor> {
  static uint32_t value(const SkColor& input) { return input; }

  static bool Read(skia::mojom::ColorDataView data, SkColor* out) {
    *out = data.value();
    return true;
  }
};

}  // namespace mojo

#endif  // SKIA_PUBLIC_INTERFACES_COLOR_STRUCT_TRAITS_H_

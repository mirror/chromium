// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/ime/arc_ime_struct_traits.h"

namespace mojo {

bool StructTraits<arc::mojom::CursorRectDataView, gfx::Rect>::Read(
    arc::mojom::CursorRectDataView data,
    gfx::Rect* out) {
  // The logic should be as same as the one in
  // components/arc/common/app_struct_traits.cc.
  if (data.right() < data.left() || data.bottom() < data.top())
    return false;

  out->SetRect(data.left(), data.top(), data.right() - data.left(),
               data.bottom() - data.top());
  return true;
}

bool StructTraits<arc::mojom::TextRangeDataView, gfx::Range>::Read(
    arc::mojom::TextRangeDataView data,
    gfx::Range* out) {
  out->set_start(data.start());
  out->set_end(data.end());
  return true;
}

}  // namespace mojo

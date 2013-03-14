// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/snapshot/snapshot.h"

#include "base/logging.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/aura/window.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/dip_util.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/rect.h"

namespace ui {

bool GrabViewSnapshot(gfx::NativeView view,
                      std::vector<unsigned char>* png_representation,
                      const gfx::Rect& snapshot_bounds) {
  return GrabWindowSnapshot(view, png_representation, snapshot_bounds);
}

bool GrabWindowSnapshot(gfx::NativeWindow window,
                        std::vector<unsigned char>* png_representation,
                        const gfx::Rect& snapshot_bounds) {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace ui

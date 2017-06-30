// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/base_focus_ring.h"

#include "ui/gfx/canvas.h"

namespace views {

bool BaseFocusRing::CanProcessEventsWithinSubtree() const {
  return false;
}

BaseFocusRing::BaseFocusRing() {
  // A layer is necessary to paint beyond the parent's bounds.
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
}

BaseFocusRing::~BaseFocusRing() {}

}  // namespace views

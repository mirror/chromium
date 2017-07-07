// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/base_focus_ring.h"

namespace views {

BaseFocusRing::BaseFocusRing() {
  // A layer is necessary to paint beyond the parent's bounds.
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  // Don't allow the view to process events.
  set_can_process_events_within_subtree(false);
}

BaseFocusRing::~BaseFocusRing() {}

}  // namespace views

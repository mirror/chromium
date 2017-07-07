// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_BASE_FOCUS_RING_H_
#define UI_VIEWS_CONTROLS_BASE_FOCUS_RING_H_

#include "ui/views/view.h"

namespace views {

// BaseFocusRing is the base class providing the core functionality for focus
// highlights which appear as a "ring" around the control or a part of the
// control. Because the focus ring may extend beyond the borders of the control,
// it uses layered, non-opaque painting.
class BaseFocusRing : public View {
 public:
  ~BaseFocusRing() override;

 protected:
  BaseFocusRing();

 private:
  DISALLOW_COPY_AND_ASSIGN(BaseFocusRing);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_BASE_FOCUS_RING_H_
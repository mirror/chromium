// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_BASE_FOCUS_RING_H_
#define UI_VIEWS_CONTROLS_BASE_FOCUS_RING_H_

#include "base/optional.h"
#include "ui/views/view.h"

namespace views {

// BaseFocusRing is the base class providing the core functionality for focus
// ring highlights.
class BaseFocusRing : public View {
 public:
  ~BaseFocusRing() override;

  // View:
  bool CanProcessEventsWithinSubtree() const override;

 protected:
  BaseFocusRing();

 private:
  DISALLOW_COPY_AND_ASSIGN(BaseFocusRing);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_BASE_FOCUS_RING_H_
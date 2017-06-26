// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_FOCUS_RING_DELEGATE_H_
#define UI_VIEWS_CONTROLS_FOCUS_RING_DELEGATE_H_

namespace gfx {

class Canvas;

}  // namespace gfx

namespace views {

class FocusRing;

// The FocusRingDelegate is implemented by controls that use the FocusRing to
// control the location/layout and painting of the focus ring.
class FocusRingDelegate {
 public:
  virtual void LayoutFocusRing(FocusRing* focus_ring) = 0;
  virtual void PaintFocusRing(FocusRing* focus_ring, gfx::Canvas* canvas) = 0;

 protected:
  virtual ~FocusRingDelegate() {}
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_FOCUS_RING_DELEGATE_H_

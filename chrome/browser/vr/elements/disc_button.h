// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_DISC_BUTTON_H_
#define CHROME_BROWSER_VR_ELEMENTS_DISC_BUTTON_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/vr/elements/button.h"
#include "ui/gfx/vector_icon_types.h"

namespace gfx {
class PointF;
}  // namespace gfx

namespace vr {

class Rect;
class VectorIcon;

// A disc button has a circle as the background and a vector icon as the
// foreground.  When hovered, background and foreground both move forward on Z
// axis.  This matches the Daydream disk-style button.
class DiscButton : public Button {
 public:
  DiscButton(base::Callback<void()> click_handler, const gfx::VectorIcon& icon);
  ~DiscButton() override;

  VectorIcon* foreground() const { return foreground_; }

  // TODO(vollick): once all elements are scaled by a ScaledDepthAdjuster, we
  // will never have to change the button hover offset from the default and this
  // method and the associated field can be removed.
  void set_hover_offset(float hover_offset) { hover_offset_ = hover_offset; }

 private:
  void OnStateUpdated() override;
  void OnSetDrawPhase() override;
  void OnSetName() override;
  void NotifyClientSizeAnimated(const gfx::SizeF& size,
                                int target_property_id,
                                cc::Animation* animation) override;

  float hover_offset_;
  VectorIcon* foreground_;
  DISALLOW_COPY_AND_ASSIGN(DiscButton);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_DISC_BUTTON_H_

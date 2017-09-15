// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_TRANSIENT_ELEMENT_H_
#define CHROME_BROWSER_VR_ELEMENTS_TRANSIENT_ELEMENT_H_

#include "chrome/browser/vr/elements/textured_element.h"

namespace vr {

class TransientElement : public UiElement {
 public:
  explicit TransientElement(const base::TimeDelta& timeout);
  ~TransientElement() override;

  void Animate(const base::TimeTicks& time) override;

  void SetVisible(bool visible) override;
  void RefreshVisible();

 private:
  typedef UiElement super;
  // We can't use UiElement::IsVisible because there may already be a scheduled
  // animation when SetVisible gets called. For example, consider the case when
  // SetVisible gets called twice in the same frame, once to set visibility and
  // the second time to unset it. IsVisible will return true the second time
  // around because the opacity transition hasn't started yet and we will stay
  // visible.
  bool will_be_visible_ = false;
  base::TimeDelta timeout_;
  base::TimeTicks set_visible_time_;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_TRANSIENT_ELEMENT_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/transient_element.h"

namespace vr {

TransientElement::TransientElement(const base::TimeDelta& timeout)
    : timeout_(timeout) {
  SetVisibleImmediately(false);
}

TransientElement::~TransientElement() {}

void TransientElement::OnBeginFrame(const base::TimeTicks& time) {
  super::OnBeginFrame(time);

  if (!IsAnimatingVisible())
    return;

  // SetVisible may have been called during initilization which means that the
  // last frame time would be zero.
  if (set_visible_time_.is_null())
    set_visible_time_ = last_frame_time();

  base::TimeDelta duration = time - set_visible_time_;
  if (duration >= timeout_)
    super::SetVisible(false);
}

void TransientElement::SetVisible(bool visible) {
  if (visible == IsAnimatingVisible())
    return;

  if (visible) {
    set_visible_time_ = last_frame_time();
    super::SetVisible(true);
  } else {
    super::SetVisible(false);
  }
}

void TransientElement::RefreshVisible() {
  if (!IsAnimatingVisible())
    return;

  set_visible_time_ = last_frame_time();
  super::SetVisible(true);
}

bool TransientElement::IsAnimatingVisible() {
  return GetTargetOpacity() == opacity_when_visible();
}

}  // namespace vr

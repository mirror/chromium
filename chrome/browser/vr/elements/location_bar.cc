// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/location_bar.h"

#include "base/memory/ptr_util.h"

namespace vr {

LocationBar::LocationBar(UiElement* background_element,
                         UiElement* thumb_element)
    : background_element_(background_element), thumb_element_(thumb_element) {
  DCHECK(background_element);
  DCHECK(thumb_element);
  AddChild(background_element);
  AddChild(thumb_element);
}

LocationBar::~LocationBar() = default;

void LocationBar::SetSize(float width, float height) {
  UiElement::SetSize(width, height);
  LayOutChildren();
}

void LocationBar::SetRange(size_t range) {
  DCHECK_GT(range, 0Lu);
  range_ = range;
  LayOutChildren();
}

void LocationBar::SetPosition(size_t position) {
  DCHECK_LT(position, range_);
  position_ = position;
  LayOutChildren();
}

size_t LocationBar::GetRange() const {
  return range_;
}

size_t LocationBar::GetPosition() const {
  return position_;
}

void LocationBar::LayOutChildren() {
  background_element_->SetTranslate(0.0f, 0.0f, 0.0f);
  background_element_->SetSize(size().width(), size().height());

  float range_width = size().width() / range_;
  float initial_pos = -size().width() / 2 + range_width / 2;
  float current_pos = initial_pos + position_ * range_width;
  thumb_element_->SetTranslate(current_pos, 0.0f, 0.005f);
}

}  // namespace vr

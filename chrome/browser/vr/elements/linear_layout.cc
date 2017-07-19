// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/linear_layout.h"

namespace vr {

LinearLayout::LinearLayout(Direction direction) : direction_(direction) {}
LinearLayout::~LinearLayout() {}

void LinearLayout::LayoutChild(UiElement* element, gfx::Transform* transform) {
  DCHECK(!children().empty());
  float offset = 0.0f;
  float total_extent = 0.0f;
  bool have_seen_element = false;
  for (auto* child : children()) {
    if (!child->visible())
      continue;
    float delta = direction_ == kHorizontal ? child->size().width()
                                            : child->size().height();
    if (child == element) {
      offset += 0.5 * delta;
      have_seen_element = true;
    }
    total_extent += delta + margin_;
    if (!have_seen_element)
      offset += delta + margin_;
  }
  total_extent -= margin_;
  DCHECK(have_seen_element);
  offset -= 0.5 * total_extent;
  if (direction_ == kHorizontal) {
    transform->matrix().postTranslate(offset, 0, 0);
  } else {
    transform->matrix().postTranslate(0, offset, 0);
  }
}

}  // namespace vr

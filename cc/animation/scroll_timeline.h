// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_SCROLL_TIMELINE_H_
#define CC_ANIMATION_SCROLL_TIMELINE_H_

#include "cc/animation/animation_export.h"
#include "cc/trees/element_id.h"
#include "cc/trees/property_tree.h"

namespace cc {

struct CC_ANIMATION_EXPORT ScrollTimeline {
  enum ScrollDirection { Block, Inline };

  ScrollTimeline(ElementId element_id,
                 ScrollDirection orientation,
                 double time_range);

  double CurrentTime(ScrollTree& scroll_tree) const;

  ElementId element_id;
  ScrollDirection orientation;
  double time_range;
};

}  // namespace cc

#endif  // CC_ANIMATION_SCROLL_TIMELINE_H_

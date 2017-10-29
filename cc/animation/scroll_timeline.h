// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_SCROLL_TIMELINE_H_
#define CC_ANIMATION_SCROLL_TIMELINE_H_

#include "cc/animation/animation_export.h"
#include "cc/trees/element_id.h"

namespace cc {

class ScrollTree;

class CC_ANIMATION_EXPORT ScrollTimeline {
 public:
  enum ScrollDirection { Horizontal, Vertical };

  ScrollTimeline(ElementId element_id,
                 ScrollDirection orientation,
                 double time_range);

  std::unique_ptr<ScrollTimeline> CreateImplInstance() const;

  double CurrentTime(const ScrollTree& scroll_tree) const;

 private:
  ElementId element_id_;
  ScrollDirection orientation_;
  double time_range_;
};

}  // namespace cc

#endif  // CC_ANIMATION_SCROLL_TIMELINE_H_

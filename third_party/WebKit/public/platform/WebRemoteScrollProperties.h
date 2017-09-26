// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebRemoteScrollProperties_h
#define WebRemoteScrollProperties_h

#include "public/platform/WebCommon.h"

#if INSIDE_BLINK
#include "platform/scroll/ScrollTypes.h"
#endif

namespace blink {

struct WebRemoteScrollProperties {
  // Scroll alignment behavior which determines how the scrolling happens with
  // respect to a certain direction.
  enum Alignment {
    kCenterIfNeeded = 0,
    kToEdgeIfNeeded,
    kCenterAlways,
    kTopAlways,
    kBottomAlways,
    kLeftAlways,
    kRightAlways,
  };

  // Scroll type set through 'scroll-type' CSS property.
  enum Type {
    kUser = 0,
    kProgrammatic,
    kClamping,
    kCompositor,
    kAnchoring,
    kSequenced,
  };

  // The scroll behavior set through 'scroll-behavior' CSS property.
  enum Behavior { kAuto = 0, kInstant, kSmooth };

  Alignment align_x = kCenterIfNeeded;
  Alignment align_y = kCenterIfNeeded;
  Type type = kProgrammatic;
  bool make_visible_in_visual_viewport = false;
  Behavior behavior = kAuto;
  bool is_for_scroll_sequence;

  WebRemoteScrollProperties() {}
#if INSIDE_BLINK
  BLINK_PLATFORM_EXPORT WebRemoteScrollProperties(Alignment,
                                                  Alignment,
                                                  ScrollType,
                                                  bool,
                                                  ScrollBehavior,
                                                  bool);

  BLINK_PLATFORM_EXPORT ScrollType GetScrollType() const;
  BLINK_PLATFORM_EXPORT ScrollBehavior GetScrollBehavior() const;
#endif
};

}  // namespace blink

#endif  // WebRemoteScrollProperties_h

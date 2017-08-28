// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef SmoothScrollSequencer_h
#define SmoothScrollSequencer_h

#include <utility>
#include "platform/heap/Handle.h"
#include "platform/scroll/ScrollTypes.h"

namespace blink {

class ScrollableArea;

struct SequencedScroll final : public GarbageCollected<SequencedScroll> {
  SequencedScroll();

  SequencedScroll(ScrollableArea* area,
                  ScrollOffset offset,
                  ScrollBehavior behavior,
                  bool user)
      : scrollable_area(area),
        scroll_offset(offset),
        scroll_behavior(behavior),
        is_user(user) {}

  SequencedScroll(const SequencedScroll& other)
      : scrollable_area(other.scrollable_area),
        scroll_offset(other.scroll_offset),
        scroll_behavior(other.scroll_behavior),
        is_user(other.is_user) {}

  Member<ScrollableArea> scrollable_area;
  ScrollOffset scroll_offset;
  ScrollBehavior scroll_behavior;
  bool is_user;

  DECLARE_TRACE();
};

// A sequencer that queues the nested scrollers from inside to outside,
// so that they can be animated from outside to inside when smooth scroll
// is called.
class PLATFORM_EXPORT SmoothScrollSequencer final
    : public GarbageCollected<SmoothScrollSequencer> {
 public:
  SmoothScrollSequencer() {}
  // Add a scroll offset animation to the back of a queue.
  void QueueAnimation(ScrollableArea*, ScrollOffset, ScrollBehavior,
                      bool is_user = false);

  // Run the animation at the back of the queue.
  void RunQueuedAnimations();

  // Abort the currently running animation and all the animations in the queue.
  void AbortAnimations();

  DECLARE_TRACE();

 private:
  HeapVector<Member<SequencedScroll>> queue_;
  Member<ScrollableArea> current_scrollable_;
};

}  // namespace blink

#endif  // SmoothScrollSequencer_h

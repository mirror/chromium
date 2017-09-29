// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/input/VRGestureEvent.h"

namespace blink {

VRGestureEvent::VRGestureEvent() {}

VRGestureEvent::VRGestureEvent(const AtomicString& type,
                               VRPresentationFrame* frame,
                               VRInputSource* input_source)
    : Event(type, true, false), frame_(frame), input_source_(input_source) {}

VRGestureEvent::VRGestureEvent(const AtomicString& type,
                               const VRGestureEventInit& initializer)
    : Event(type, initializer) {
  if (initializer.hasFrame())
    frame_ = initializer.frame();
  if (initializer.hasInputSource())
    input_source_ = initializer.inputSource();
}

VRGestureEvent::~VRGestureEvent() {}

const AtomicString& VRGestureEvent::InterfaceName() const {
  return EventNames::VRGestureEvent;
}

DEFINE_TRACE(VRGestureEvent) {
  visitor->Trace(frame_);
  visitor->Trace(input_source_);
  Event::Trace(visitor);
}

}  // namespace blink

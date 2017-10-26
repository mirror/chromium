/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/events/PageTransitionEvent.h"

namespace blink {

namespace {
AtomicString TransitionReasonToString(
    PageTransitionEvent::TransitionReason reason) {
  switch (reason) {
    case PageTransitionEvent::TransitionReason::kUnknown:
      return "unknown";
    case PageTransitionEvent::TransitionReason::kStopped:
      return "stopped";
    case PageTransitionEvent::TransitionReason::kDiscarded:
      return "discarded";
  }
  NOTREACHED();
  return "";
}

PageTransitionEvent::TransitionReason TransitionReasonFromString(
    const String& reason) {
  if (reason == "unknown")
    return PageTransitionEvent::TransitionReason::kUnknown;
  if (reason == "stopped")
    return PageTransitionEvent::TransitionReason::kStopped;
  if (reason == "discarded")
    return PageTransitionEvent::TransitionReason::kDiscarded;
  return PageTransitionEvent::TransitionReason::kUnknown;
}
}  // namespace

PageTransitionEvent::PageTransitionEvent()
    : persisted_(false), reason_(TransitionReason::kUnknown) {}

PageTransitionEvent::PageTransitionEvent(const AtomicString& type,
                                         bool persisted) {
  PageTransitionEvent(type, persisted, TransitionReason::kUnknown);
}

PageTransitionEvent::PageTransitionEvent(const AtomicString& type,
                                         bool persisted,
                                         TransitionReason reason)
    : Event(type, true, true), persisted_(persisted), reason_(reason) {}

PageTransitionEvent::PageTransitionEvent(
    const AtomicString& type,
    const PageTransitionEventInit& initializer)
    : Event(type, initializer), persisted_(false) {
  if (initializer.hasPersisted())
    persisted_ = initializer.persisted();
  if (initializer.hasReason())
    reason_ = TransitionReasonFromString(initializer.reason());
}

PageTransitionEvent::~PageTransitionEvent() {}

const AtomicString& PageTransitionEvent::InterfaceName() const {
  return EventNames::PageTransitionEvent;
}

AtomicString PageTransitionEvent::reason() const {
  return TransitionReasonToString(reason_);
}

void PageTransitionEvent::Trace(blink::Visitor* visitor) {
  Event::Trace(visitor);
}

}  // namespace blink

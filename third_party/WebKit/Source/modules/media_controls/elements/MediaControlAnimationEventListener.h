// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaControlAnimationEventListener_h
#define MediaControlAnimationEventListener_h

#include "core/dom/events/EventListener.h"
#include "modules/ModulesExport.h"
#include "modules/media_controls/elements/MediaControlDivElement.h"
#include "platform/heap/GarbageCollected.h"

namespace blink {

class ExecutionContext;
class Event;
class HTMLElement;

// Listens for animationend and animationiteration DOM events on a HTML element
// provided by the loading panel. When the events are called it calls the
// OnAnimation* methods on the loading panel.
//
// This exists because we need to know when the animation ends so we can reset
// the element and we also need to keep track of how many iterations the
// animation has gone through so we can nicely stop the animation at the end of
// the current one.
class MODULES_EXPORT MediaControlAnimationEventListener final
    : public EventListener {
 public:
  class MODULES_EXPORT Observer : public GarbageCollectedMixin {
   public:
    virtual void OnAnimationIteration() = 0;
    virtual void OnAnimationEnd() = 0;

    virtual HTMLElement& WatchedAnimationElement() const = 0;

    void Trace(blink::Visitor*);
  };

  explicit MediaControlAnimationEventListener(Observer*);
  void Detach();

  bool operator==(const EventListener& other) const override;

  void Trace(Visitor*) override;

 private:
  void handleEvent(ExecutionContext*, Event*);

  Member<Observer> observer_;
};

}  // namespace blink

#endif  // MediaControlAnimationEventListener_h

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/event_processor.h"

#include "ui/events/event_target.h"
#include "ui/events/event_targeter.h"

namespace ui {

EventProcessor::EventProcessor() : weak_ptr_factory_(this) {}

EventProcessor::~EventProcessor() {}

EventDispatchDetails EventProcessor::OnEventFromSource(Event* event) {
  static int msw_static = 0;
  msw_static++;
  int msw = msw_static;

  LOG(ERROR) << "MSW OnEventFromSource A" << msw; 
  base::WeakPtr<EventProcessor> weak_this = weak_ptr_factory_.GetWeakPtr();
  // If |event| is in the process of being dispatched or has already been
  // dispatched, then dispatch a copy of the event instead. We expect event
  // target to be already set if event phase is after EP_PREDISPATCH.
  bool dispatch_original_event = event->phase() == EP_PREDISPATCH;
  DCHECK(dispatch_original_event || event->target());
  Event* event_to_dispatch = event;
  std::unique_ptr<Event> event_copy;
  if (!dispatch_original_event) {
    event_copy = Event::Clone(*event);
    event_to_dispatch = event_copy.get();
  }

  EventDispatchDetails details;
  OnEventProcessingStarted(event_to_dispatch);
  LOG(ERROR) << "MSW OnEventFromSource AA" << msw; 
  // GetInitialEventTarget() may handle the event.
  EventTarget* initial_target = event_to_dispatch->handled()
                                    ? nullptr
                                    : GetInitialEventTarget(event_to_dispatch);
  LOG(ERROR) << "MSW OnEventFromSource AB" << msw << " " << event_to_dispatch->handled() << " " << initial_target; 
  if (!event_to_dispatch->handled()) {
    EventTarget* target = initial_target;
    EventTargeter* targeter = nullptr;

    if (!target) {
      EventTarget* root = GetRootForEvent(event_to_dispatch);
      DCHECK(root);
      targeter = root->GetEventTargeter();
      if (targeter) {
        target = targeter->FindTargetForEvent(root, event_to_dispatch);
      } else {
        targeter = GetDefaultEventTargeter();
        // LOG(ERROR) << "MSW OnEventFromSource E" << msw << " " << event_to_dispatch->target(); 
        if (event_to_dispatch->target())
          target = root;
        else
          target = targeter->FindTargetForEvent(root, event_to_dispatch);
      }
      DCHECK(targeter);
    }

    while (target) {
      details = DispatchEvent(target, event_to_dispatch);

      if (!dispatch_original_event) {
        if (event_to_dispatch->stopped_propagation())
          event->StopPropagation();
        else if (event_to_dispatch->handled())
          event->SetHandled();
      }

      if (details.dispatcher_destroyed)
        return details;

      if (!weak_this) {
        details.dispatcher_destroyed = true;
        return details;
      }

      if (details.target_destroyed || event->handled() ||
          target == initial_target) {
        break;
      }

      DCHECK(targeter);
      target = targeter->FindNextBestTarget(target, event_to_dispatch);
    }
  }
  OnEventProcessingFinished(event);
  LOG(ERROR) << "MSW OnEventFromSource Z" << msw; 
  return details;
}

void EventProcessor::OnEventProcessingStarted(Event* event) {
}

void EventProcessor::OnEventProcessingFinished(Event* event) {
}

}  // namespace ui

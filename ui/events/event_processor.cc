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
  return ProcessEvent(weak_ptr_factory_.GetWeakPtr(), event);
}

EventDispatchDetails EventProcessor::ProcessEvent(
    base::WeakPtr<EventProcessor> event_processor,
    Event* event) {
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
  event_processor->OnEventProcessingStarted(event_to_dispatch);

  // GetInitialEventTarget() may handle the event.
  EventTarget* initial_target =
      event_to_dispatch->handled()
          ? nullptr
          : event_processor->GetInitialEventTarget(event_to_dispatch);
  if (!event_to_dispatch->handled()) {
    EventTarget* target = initial_target;
    EventTargeter* targeter = nullptr;

    if (!target) {
      EventTarget* root = event_processor->GetRootForEvent(event_to_dispatch);
      DCHECK(root);
      targeter = root->GetEventTargeter();
      if (targeter) {
        target = targeter->FindTargetForEvent(root, event_to_dispatch);
      } else {
        targeter = event_processor->GetDefaultEventTargeter();
        if (event_to_dispatch->target())
          target = root;
        else
          target = targeter->FindTargetForEvent(root, event_to_dispatch);
      }
      DCHECK(targeter);
    }

    while (target) {
      details = event_processor->DispatchEvent(target, event_to_dispatch);

      if (!dispatch_original_event) {
        if (event_to_dispatch->stopped_propagation())
          event->StopPropagation();
        else if (event_to_dispatch->handled())
          event->SetHandled();
      }

      if (details.dispatcher_destroyed)
        return details;

      if (!event_processor) {
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

  event_processor->OnEventProcessingFinished(event);
  return details;
}

void EventProcessor::OnEventProcessingStarted(Event* event) {
}

void EventProcessor::OnEventProcessingFinished(Event* event) {
}

}  // namespace ui

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/test_event_router.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/extension_prefs.h"

namespace extensions {

TestEventRouter::EventObserver::~EventObserver() {}

void TestEventRouter::EventObserver::OnDispatchEventToExtension(
    const std::string& extension_id,
    const Event& event) {}

void TestEventRouter::EventObserver::OnBroadcastEvent(const Event& event) {}

TestEventRouter::TestEventRouter(content::BrowserContext* context)
    : EventRouter(context, ExtensionPrefs::Get(context)) {}

TestEventRouter::~TestEventRouter() {}

int TestEventRouter::GetEventCount(std::string event_name) const {
  if (seen_events_.count(event_name) == 0)
    return 0;
  return seen_events_.find(event_name)->second;
}

void TestEventRouter::WaitForEvent(const std::string& event_name) {
  CHECK(!event_name.empty());
  CHECK(wait_for_event_name_.empty());
  CHECK(!run_loop_ || !run_loop_->running());

  wait_for_event_name_ = event_name;
  run_loop_ = std::make_unique<base::RunLoop>();
  run_loop_->Run();
}

void TestEventRouter::AddEventObserver(EventObserver* obs) {
  observers_.AddObserver(obs);
}

void TestEventRouter::RemoveEventObserver(EventObserver* obs) {
  observers_.RemoveObserver(obs);
}

void TestEventRouter::BroadcastEvent(std::unique_ptr<Event> event) {
  IncrementEventCount(event->event_name);

  for (auto& observer : observers_)
    observer.OnBroadcastEvent(*event);
}

void TestEventRouter::DispatchEventToExtension(const std::string& extension_id,
                                               std::unique_ptr<Event> event) {
  if (!expected_extension_id_.empty())
    DCHECK_EQ(expected_extension_id_, extension_id);

  IncrementEventCount(event->event_name);

  for (auto& observer : observers_)
    observer.OnDispatchEventToExtension(extension_id, *event);

  if (event->event_name == wait_for_event_name_ && run_loop_->running()) {
    wait_for_event_name_.clear();
    run_loop_->Quit();
  }
}

void TestEventRouter::IncrementEventCount(const std::string& event_name) {
  if (seen_events_.count(event_name) == 0)
    seen_events_[event_name] = 0;
  seen_events_[event_name]++;
}

}  // namespace extensions

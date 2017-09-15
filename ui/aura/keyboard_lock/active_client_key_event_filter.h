// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_KEYBOARD_LOCK_ACTIVE_CLIENT_KEY_EVENT_FILTER_H_
#define UI_AURA_KEYBOARD_LOCK_ACTIVE_CLIENT_KEY_EVENT_FILTER_H_

#include "ui/aura/keyboard_lock/active_client_tracker.h"
#include "ui/events/event.h"
#include "ui/events/platform/key_event_filter.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

// Forwards the ui::KeyEvent to the active client in ActiveClientTracker.
template <typename CLIENT_ID>
class ActiveClientKeyEventFilter final : public ui::KeyEventFilter {
 public:
  explicit ActiveClientKeyEventFilter(
      const ActiveClientTracker<CLIENT_ID>& tracker)
      : tracker_(tracker) {}
  ~ActiveClientKeyEventFilter() override = default;

  bool OnKeyEvent(const ui::KeyEvent& event) override {
    KeyEventFilter* filter = tracker_.GetActiveKeyEventFilter();
    if (!filter) {
      LOG(ERROR) << "No active KeyEventFilter";
    }
    return filter && filter->OnKeyEvent(event);
  }

 private:
  const ActiveClientTracker<CLIENT_ID>& tracker_;
};

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

#endif  // UI_AURA_KEYBOARD_LOCK_ACTIVE_CLIENT_KEY_EVENT_FILTER_H_

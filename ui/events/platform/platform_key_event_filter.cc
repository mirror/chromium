// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/platform/platform_key_event_filter.h"

#include "base/logging.h"
#include "ui/events/platform/key_event_filter.h"
#include "ui/events/event.h"
#include "ui/events/event_utils.h"

namespace ui {

PlatformKeyEventFilter::PlatformKeyEventFilter(KeyEventFilter* filter)
    : filter_(filter) {
  DCHECK(filter_);
}
PlatformKeyEventFilter::~PlatformKeyEventFilter() = default;

bool PlatformKeyEventFilter::OnPlatformEvent(const ui::PlatformEvent& event) {
  const ui::EventType event_type = ui::EventTypeFromNative(event);
  if (event_type != ui::ET_KEY_PRESSED && event_type != ui::ET_KEY_RELEASED) {
    return false;
  }

  // TODO(zijiehe): Now ui::KeyEvent::KeyEvent(const base::NativeEvent&)
  // constructor automatically set IsRepeated() field, which impacts the
  // PlatformKeyEventFilter, since we may eventually generate two ui::KeyEvent
  // instances from one ui::PlatformEvent. This should be fixed by moving the
  // repeating detection out of ui::KeyEvent.
  // Now PlatformEventKeyFilter won't be able to set ui::KeyEvent::is_repeat()
  // correctly.
  const ui::KeyboardCode key_code = ui::KeyboardCodeFromNative(event);
  const ui::DomCode dom_code = ui::CodeFromNative(event);
  const int flags = ui::EventFlagsFromNative(event);
  ui::KeyEvent key_event(event_type, key_code, dom_code, flags);
  return filter_->OnKeyEvent(key_event);
}

}  // namespace ui

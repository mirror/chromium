// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/platform/platform_key_event_filter.h"

#include "base/logging.h"
#include "ui/events/platform/key_event_filter.h"
#include "ui/events/event_utils.h"

namespace ui {

PlatformKeyEventFilter::PlatformKeyEventFilter(KeyEventFilter* filter)
    : filter_(filter) {
  DCHECK(filter_);
}
PlatformKeyEventFilter::~PlatformKeyEventFilter() = default;

bool PlatformKeyEventFilter::OnPlatformEvent(const ui::PlatformEvent& event) {
  ui::EventType event_type = ui::EventTypeFromNative(event);
  if (event_type != ui::ET_KEY_PRESSED && event_type != ui::ET_KEY_RELEASED) {
    return false;
  }

  ui::KeyboardCode code = ui::KeyboardCodeFromNative(event);
  int flags = ui::EventFlagsFromNative(event);
  bool result = (event_type == ui::ET_KEY_PRESSED ?
                 filter_->OnKeyDown(code, flags) :
                 filter_->OnKeyUp(code, flags));
  return result;
}

}  // namespace ui

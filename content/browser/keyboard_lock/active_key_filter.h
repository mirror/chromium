// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_KEYBOARD_LOCK_ACTIVE_KEY_FILTER_H_
#define CONTENT_BROWSER_KEYBOARD_LOCK_ACTIVE_KEY_FILTER_H_

#include "ui/events/event.h"
#include "ui/events/platform/key_event_filter.h"

namespace content {

// TODO: COMMENT!
class ActiveKeyFilter final : public ui::KeyEventFilter {
 public:
  ActiveKeyFilter();
  ~ActiveKeyFilter() override;

  bool OnKeyEvent(const ui::KeyEvent& event) override;
};

}  // namespace content

#endif  // CONTENT_BROWSER_KEYBOARD_LOCK_ACTIVE_KEY_FILTER_H_

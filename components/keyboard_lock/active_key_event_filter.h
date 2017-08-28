// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_ACTIVE_KEY_EVENT_FILTER_H_
#define COMPONENTS_KEYBOARD_LOCK_ACTIVE_KEY_EVENT_FILTER_H_

#include "components/keyboard_lock/key_event_filter.h"

namespace keyboard_lock {

class ActiveKeyEventFilterTracker;

// A KeyEventFilter implementation to forward the key events to the active
// KeyEventFilter in |tracker|.
class ActiveKeyEventFilter final : public KeyEventFilter {
 public:
  // |tracker| must outlive |this|.
  ActiveKeyEventFilter(const ActiveKeyEventFilterTracker* tracker);
  ~ActiveKeyEventFilter() override;

 private:
  // KeyEventFilter implementations.
  bool OnKeyDown(ui::KeyboardCode code, int flags) override;
  bool OnKeyUp(ui::KeyboardCode code, int flags) override;

  const ActiveKeyEventFilterTracker* const tracker_;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_ACTIVE_KEY_EVENT_FILTER_H_

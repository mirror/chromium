// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_KEY_EVENT_FILTER_SHARE_WRAPPER_H_
#define COMPONENTS_KEYBOARD_LOCK_KEY_EVENT_FILTER_SHARE_WRAPPER_H_

#include "components/keyboard_lock/key_event_filter.h"

namespace keyboard_lock {

class KeyEventFilterShareWrapper final : public KeyEventFilter {
 public:
  KeyEventFilterShareWrapper(KeyEventFilter* filter);
  ~KeyEventFilterShareWrapper() override;

 private:
  // KeyEventFilter implementations
  bool OnKeyDown(ui::KeyboardCode code, int flags) override;
  bool OnKeyUp(ui::KeyboardCode code, int flags) override;

  KeyEventFilter* const filter_;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_KEY_EVENT_FILTER_SHARE_WRAPPER_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/key_event_filter_share_wrapper.h"

#include "base/logging.h"

namespace keyboard_lock {

KeyEventFilterShareWrapper::KeyEventFilterShareWrapper(KeyEventFilter* filter)
    : filter_(filter) {
  DCHECK(filter_);
}

KeyEventFilterShareWrapper::~KeyEventFilterShareWrapper() = default;

bool KeyEventFilterShareWrapper::OnKeyDown(ui::KeyboardCode code, int flags) {
  return filter_->OnKeyDown(code, flags);
}

bool KeyEventFilterShareWrapper::OnKeyUp(ui::KeyboardCode code, int flags) {
  return filter_->OnKeyUp(code, flags);
}

}  // namespace keyboard_lock

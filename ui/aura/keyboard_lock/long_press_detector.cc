// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/logging.h"
#include "base/time/time.h"
#include "ui/aura/keyboard_lock/long_press_detector.h"
#include "ui/events/keycodes/dom/keycode_converter.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

LongPressDetector::LongPressDetector(NativeKeyCode code,
                                     int timeout_ms,
                                     Delegate* delegate,
                                     std::unique_ptr<Client> client)
    : code_(code),
      timeout_ms_(timeout_ms),
      delegate_(delegate),
      client_(std::move(client)) {
  DCHECK_GT(timeout_ms_, 0);
  DCHECK(delegate_);
  DCHECK(client_);
}

LongPressDetector::~LongPressDetector() = default;

void LongPressDetector::OnKeyEvent(const ui::KeyEvent& event) {
  if (code_ == KeycodeConverter::DomCodeToNativeKeycode(event.code()) &&
      event.type() == ET_KEY_PRESSED) {
    const base::TimeTicks now = base::TimeTicks::Now();
    if (first_down_ms_ > now) {
      first_down_ms_ = now;
    } else if ((now - first_down_ms_).InMilliseconds() >= timeout_ms_ &&
               (now - first_down_ms_).InMilliseconds() < timeout_ms_ * 2) {
      delegate_->LongPressDetected();
      first_down_ms_ = now;
      return;
    }
  } else {
    first_down_ms_ = base::TimeTicks::Max();
  }
  client_->OnKeyEvent(event);
}

LongPressDetector::Delegate::Delegate() = default;
LongPressDetector::Delegate::~Delegate() = default;

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

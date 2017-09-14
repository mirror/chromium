// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/logging.h"
#include "base/time/time.h"
#include "ui/aura/keyboard_lock/special_key_detector.h"
#include "ui/events/keycodes/dom/keycode_converter.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

SpecialKeyDetector::SpecialKeyDetector(Delegate* delegate,
                                       std::unique_ptr<Client> client)
    : delegate_(delegate),
      client_(std::move(client)) {
  DCHECK(delegate_);
  DCHECK(client_);
  DCHECK_GT(delegate_->long_press_ms(), 0);
  DCHECK_GT(delegate_->repeat_times(), 1);
  DCHECK_GT(delegate_->repeat_timeout_ms(), 0);
}

SpecialKeyDetector::~SpecialKeyDetector() = default;

void SpecialKeyDetector::OnKeyEvent(const ui::KeyEvent& event) {
  if (delegate_->code() ==
      KeycodeConverter::DomCodeToNativeKeycode(event.code())) {
    const base::TimeTicks now = base::TimeTicks::Now();
    if (event.type() == ET_KEY_PRESSED) {
      RemoveDeprecatedReleaseTimes();
      delegate_->PressDetected(static_cast<int>(release_times_.size()));
      if (press_time_ > now) {
        press_time_ = now;
      } else if ((now - press_time_).InMilliseconds() >=
                 delegate_->long_press_ms()) {
        // Note: if Client was blured after the key was pressed, and gets focus
        // after long_press_ms() with the key down, the LongPressDetected()
        // event will still be fired.
        // In general, this should not be a problem, but this behavior should be
        // awared. Notifying Client in Observer can resolve the issue, but it
        // seems not worthy.
        delegate_->LongPressDetected();
        press_time_ = now;
      }
    } else {
      DCHECK_EQ(event.type(), ET_KEY_RELEASED);
      press_time_ = base::TimeTicks::Max();
      RemoveDeprecatedReleaseTimes();
      release_times_.push(now);
      DCHECK(!release_times_.empty());
      if (static_cast<int>(release_times_.size()) ==
          delegate_->repeat_times()) {
        delegate_->RepeatPressDetected();
        release_times_ = std::queue<base::TimeTicks>();
      }
    }
  } else {
    press_time_ = base::TimeTicks::Max();
    release_times_ = std::queue<base::TimeTicks>();
  }
  client_->OnKeyEvent(event);
}

void SpecialKeyDetector::RemoveDeprecatedReleaseTimes() {
  const base::TimeTicks now = base::TimeTicks::Now();
  while (!release_times_.empty() &&
         (now - release_times_.front()).InMilliseconds() >
         delegate_->repeat_timeout_ms()) {
    release_times_.pop();
  }
}

SpecialKeyDetector::Delegate::Delegate() = default;
SpecialKeyDetector::Delegate::~Delegate() = default;

int SpecialKeyDetector::Delegate::long_press_ms() const { return 2000; }
int SpecialKeyDetector::Delegate::repeat_times() const { return 5; }
int SpecialKeyDetector::Delegate::repeat_timeout_ms() const { return 3000; }
void SpecialKeyDetector::Delegate::LongPressDetected() {}
void SpecialKeyDetector::Delegate::RepeatPressDetected() {}
void SpecialKeyDetector::Delegate::PressDetected(int repeated_count) {}

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

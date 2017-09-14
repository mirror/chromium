// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_KEYBOARD_LOCK_SPECIAL_KEY_DETECTOR_H_
#define UI_AURA_KEYBOARD_LOCK_SPECIAL_KEY_DETECTOR_H_

#include <stdint.h>

#include <memory>
#include <queue>

#include "base/time/time.h"
#include "ui/aura/keyboard_lock/client.h"
#include "ui/aura/keyboard_lock/types.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

// A wrapper of Client to detect the pressing of a key.
class SpecialKeyDetector final : public Client {
 public:
  class Delegate {
   public:
    Delegate();
    virtual ~Delegate();

    // Configuration of the SpecialKeyDetector. ===============================
    // The return values expected to be not changed during the lifetime of the
    // SpecialKeyDetector.
    // The interesting key.
    virtual NativeKeyCode code() const = 0;

    // The milliseconds considered as a long-press of the key.
    virtual int long_press_ms() const;

    // The times of pressings considered as a repeat-press of the key.
    virtual int repeat_times() const;

    // The milliseconds considered as repeat-press of the key.
    virtual int repeat_timeout_ms() const;

    // Events of the SpeicialKeyDetector. =====================================
    // Will be fired if code() has been pressed over long_press_ms(). This event
    // will be fired only once per long_press_ms().
    virtual void LongPressDetected();

    // Will be fired if repeated count reaches repeat_times(). This event will
    // be fired only once per repeat_times() pressings. Note, this event detects
    // the key-release.
    virtual void RepeatPressDetected();

    // Will be fired each time code() key is pressed. This event only considers
    // the pressings within repeat_times(). |repeated_count| starts from 0, i.e.
    // the first PressDetected() event will be fired with 0.
    virtual void PressDetected(int repeated_count);
  };

  SpecialKeyDetector(Delegate* delegate, std::unique_ptr<Client> client);
  ~SpecialKeyDetector() override;

  void OnKeyEvent(const ui::KeyEvent& event) override;

 private:
  void RemoveDeprecatedReleaseTimes();

  Delegate* const delegate_;
  std::unique_ptr<Client> client_;
  // The time when the special key was pressed.
  base::TimeTicks press_time_ = base::TimeTicks::Max();
  // The list of times when the special key was released within last
  // repeated_timeout_ms().
  std::queue<base::TimeTicks> release_times_;
};

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

#endif  // UI_AURA_KEYBOARD_LOCK_SPECIAL_KEY_DETECTOR_H_

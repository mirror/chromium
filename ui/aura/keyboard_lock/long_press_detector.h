// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_KEYBOARD_LOCK_LONG_PRESS_DETECTOR_H_
#define UI_AURA_KEYBOARD_LOCK_LONG_PRESS_DETECTOR_H_

#include <stdint.h>

#include <memory>

#include "base/time/time.h"
#include "ui/aura/keyboard_lock/client.h"
#include "ui/aura/keyboard_lock/types.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

// A wrapper of Client to detect the long-press of a key.
class LongPressDetector final : public Client {
 public:
  class Delegate {
   public:
    Delegate();
    virtual ~Delegate();

    virtual void LongPressDetected() = 0;
  };

  LongPressDetector(NativeKeyCode code,
                    int timeout_ms,
                    Delegate* delegate,
                    std::unique_ptr<Client> client);
  ~LongPressDetector() override;

  void OnKeyEvent(const ui::KeyEvent& event) override;

 private:
  const NativeKeyCode code_;
  const int timeout_ms_;
  Delegate* const delegate_;
  std::unique_ptr<Client> client_;
  base::TimeTicks first_down_ms_ = base::TimeTicks::Max();
};

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

#endif  // UI_AURA_KEYBOARD_LOCK_LONG_PRESS_DETECTOR_H_

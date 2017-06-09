// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_MAXIMIZE_MODE_TOUCHPAD_DISABLER_H_
#define ASH_WM_MAXIMIZE_MODE_TOUCHPAD_DISABLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/shell_observer.h"
#include "ash/wm/maximize_mode/touchpad_disabler_delegate.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace ash {

// TouchpadDisabler is an implementation detail of
// ScopedDisableInternalMouseAndKeyboardOzone. It handles disabling touchpad
// state. TouchpadDisabler may need to outlive
// ScopedDisableInternalMouseAndKeyboardOzone, as such the destructor is
// privated. Use Destroy() to signal the TouchpadDisabler should be destroyed.
// Calling Destroy() deletes immediately if not awaiting an ack, otherwise
// destruction happens after the ack is received, or when the Shell is
// destroyed, whichever comes first.
class ASH_EXPORT TouchpadDisabler : public ShellObserver {
 public:
  // TouchpadDisablerDelegate is only useful for tests, otherwise supply null,
  // which creates the appropriate implementation.
  TouchpadDisabler(
      std::unique_ptr<TouchpadDisablerDelegate> delegate = nullptr);

  void Destroy();

 private:
  ~TouchpadDisabler() override;

  // Callback from running the disable closure.
  void OnDisableAck(bool suceeded);

  // ShellObserver:
  void OnShellDestroyed() override;

  std::unique_ptr<TouchpadDisablerDelegate> delegate_;

  // Set to true when the ack from the DisableClosure is received.
  bool got_ack_ = false;

  // Set to true in OnDisableAck().
  bool did_disable_ = false;

  // Set to true in Destroy(). Indicates |this| should be deleted when
  // OnDisableAck() is called.
  bool delete_on_ack_ = false;

  base::WeakPtrFactory<TouchpadDisabler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TouchpadDisabler);
};

}  // namespace ash

#endif  // ASH_WM_MAXIMIZE_MODE_TOUCHPAD_DISABLER_H_

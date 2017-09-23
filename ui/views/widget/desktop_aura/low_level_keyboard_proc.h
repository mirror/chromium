// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_DESKTOP_AURA_LOW_LEVEL_KEYBOARD_PROC_H_
#define UI_VIEWS_WIDGET_DESKTOP_AURA_LOW_LEVEL_KEYBOARD_PROC_H_

#include <windows.h>

#include "ui/views/views_export.h"

namespace base {
template <typename Type>
struct DefaultSingletonTraits;
}  // namespace base

namespace views {

class HWNDMessageHandlerDelegate;

class VIEWS_EXPORT LowLevelKeyboardProc final {
 public:
  static LowLevelKeyboardProc* GetInstance();

  // Requests OS to reserve keys, the keys will be delivered to |callback|.
  bool LockKeys(HWNDMessageHandlerDelegate* callback);

  // Cancels the reserved keys if |callback| is |callback_|. This ensures no
  // conflict when focus is changed between two PlatformWindows.
  bool UnlockKeys(HWNDMessageHandlerDelegate* callback);

 private:
  friend struct base::DefaultSingletonTraits<LowLevelKeyboardProc>;

  static LRESULT ProcessCallback(int code,
                                 WPARAM w_param,
                                 LPARAM l_param);

  bool ProcessKeyEvent(WPARAM w_param, KBDLLHOOKSTRUCT* ll_hooks);

  // This is a singleton object.
  LowLevelKeyboardProc();
  ~LowLevelKeyboardProc();

  HHOOK hook_ = NULL;
  HWNDMessageHandlerDelegate* callback_ = nullptr;
};

}  // namespace views

#endif  // UI_VIEWS_WIDGET_DESKTOP_AURA_LOW_LEVEL_KEYBOARD_PROC_H_

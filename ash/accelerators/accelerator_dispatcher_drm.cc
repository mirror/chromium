// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accelerators/accelerator_dispatcher.h"

#include "ash/accelerators/accelerator_controller.h"
#include "ash/shell.h"
#include "ash/ime/event.h"
#include "ui/aura/event.h"
#include "ui/aura/env.h"
#include "ui/aura/root_window.h"
#include "ui/base/accelerators/accelerator.h"

namespace ash {

namespace {

const int kModifierMask = (ui::EF_SHIFT_DOWN |
                           ui::EF_CONTROL_DOWN |
                           ui::EF_ALT_DOWN);
}  // namespace

base::MessagePumpDispatcher::DispatchStatus AcceleratorDispatcher::Dispatch(
    base::NativeEvent ev) {
  // TODO(oshima): Consolidate win and linux.  http://crbug.com/116282
  if (!associated_window_)
    return EVENT_QUIT;
  if (!ui::IsNoopEvent(ev) && !associated_window_->CanReceiveEvents())
    return aura::Env::GetInstance()->GetDispatcher()->Dispatch(ev);

  if (ui::EventTypeFromNative(ev) == ui::ET_KEY_PRESSED ||
      ui::EventTypeFromNative(ev) == ui::ET_KEY_RELEASED) {
    ash::AcceleratorController* accelerator_controller =
        ash::Shell::GetInstance()->accelerator_controller();
    if (accelerator_controller) {
      ui::Accelerator accelerator(ui::KeyboardCodeFromNative(ev),
          ui::EventFlagsFromNative(ev) & kModifierMask);
      if (ui::EventTypeFromNative(ev) == ui::ET_KEY_RELEASED)
        accelerator.set_type(ui::ET_KEY_RELEASED);
      if (accelerator_controller->Process(accelerator))
        return EVENT_PROCESSED;

      accelerator.set_type(TranslatedKeyEvent(ev, false).type());
      if (accelerator_controller->Process(accelerator))
        return EVENT_PROCESSED;
    }
  }
  return nested_dispatcher_->Dispatch(ev);
}

}  // namespace ash

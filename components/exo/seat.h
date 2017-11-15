// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_SEAT_H_
#define COMPONENTS_EXO_SEAT_H_

#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "ui/aura/client/drag_drop_delegate.h"
#include "ui/aura/client/focus_change_observer.h"

namespace exo {

class Keyboard;

// Seat object managing input devices.
class Seat : public base::RefCounted<Seat>,
             public aura::client::FocusChangeObserver {
 public:
  Seat();

  void AddKeyboard(Keyboard* keyboard);
  void RemoveKeyboard(Keyboard* keyboard);

  // Overridden from aura::client::FocusChangeObserver:
  void OnWindowFocused(aura::Window* gained_focus,
                       aura::Window* lost_focus) override;

 private:
  friend class base::RefCounted<Seat>;

  ~Seat() override;

  base::ObserverList<Keyboard> keyboards_;

  DISALLOW_COPY_AND_ASSIGN(Seat);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_SEAT_H_

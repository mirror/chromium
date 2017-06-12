// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_IME_IME_CONTROLLER_H_
#define ASH_IME_IME_CONTROLLER_H_

#include <vector>

#include "ash/ash_export.h"
#include "ash/public/interfaces/ime_info.mojom.h"
#include "base/macros.h"

namespace ash {

// Connects ash IME users (e.g. the system tray) to the IME implementation,
// which might live in Chrome browser or in a separate mojo service.
// TODO(jamescook): Convert to use mojo IME interface to Chrome browser.
class ASH_EXPORT ImeController {
 public:
  ImeController();
  ~ImeController();

  //JAMES TODO const& ify everything

  // Returns the currently selected IME.
  mojom::ImeInfo GetCurrentIme() const;

  // Returns a list of available IMEs. "Available" IMEs are both installed and
  // enabled by the user in settings.
  std::vector<mojom::ImeInfo> GetAvailableImes() const;

  // Returns true if the available IMEs are managed by enterprise policy.
  bool IsImeManaged() const;

  // Returns additional menu items for properties of the currently selected IME.
  std::vector<mojom::ImeMenuItem> GetCurrentImeMenuItems() const;

  void OnCurrentImeChanged(const mojom::ImeInfo& ime);

  void OnAvailableImesChanged(const std::vector<mojom::ImeInfo>& imes,
                              bool managed_by_policy);

  void OnCurrentImeMenuItemsChanged(
      const std::vector<mojom::ImeMenuItem>& items);

  void ShowImeMenuOnShelf(bool show);

 private:
  mojom::ImeInfo current_ime_;
  std::vector<mojom::ImeInfo> available_imes_;
  bool managed_by_policy_ = false;
  std::vector<mojom::ImeMenuItem> current_ime_menu_items_;

  DISALLOW_COPY_AND_ASSIGN(ImeController);
};

}  // namespace ash

#endif  // ASH_IME_IME_CONTROLLER_H_

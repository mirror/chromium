// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_IME_IME_MANAGER_H_
#define ASH_IME_IME_MANAGER_H_

#include <vector>

#include "ash/ash_export.h"
#include "base/macros.h"

namespace ash {

namespace mojom {
class ImeInfo;
class ImeProperty;
}  // namespace mojom

// Connects ash IME users (e.g. the system tray) to the IME implementation,
// which might live in Chrome browser or in a separate mojo service.
// TODO(jamescook): Convert to use mojo IME interface to Chrome browser.
class ASH_EXPORT ImeManager {
 public:
  ImeManager();
  virtual ~ImeManager();

  // Returns the currently selected IME.
  virtual mojom::ImeInfo GetCurrentIme() const;

  // Returns a list of properties for the currently selected IME.
  virtual std::vector<mojom::ImeProperty> GetCurrentImeProperties() const;

  // Returns a list of available IMEs. "Available" IMEs are both installed and
  // enabled by the user in settings.
  virtual std::vector<mojom::ImeInfo> GetAvailableImes() const;

  // Returns true if the available IMEs are managed by enterprise policy.
  virtual bool IsImeManaged() const;

 private:
  DISALLOW_COPY_AND_ASSIGN(ImeManager);
};

}  // namespace ash

#endif  // ASH_IME_IME_MANAGER_H_

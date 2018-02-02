// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/crostini/crostini_util.h"

#include "chrome/browser/chromeos/settings/cros_settings.h"

namespace crostini {

bool IsCrostiniAllowed() {
  bool crostini_allowed;
  if (chromeos::CrosSettings::Get()->GetBoolean(chromeos::kCrostiniAllowed,
                                                &crostini_allowed)) {
    return crostini_allowed;
  }
  // If device policy is not set, allow Crostini.
  return true;
}

}  // namespace crostini

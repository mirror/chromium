// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/win/did_run_util.h"

#include "base/command_line.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/install_static/install_modes.h"

bool ShouldRecordActiveUse(const base::CommandLine& command_line) {
  if (!install_static::kUseGoogleUpdateIntegration)
    return false;
  return !command_line.HasSwitch(switches::kTryChromeAgain);
}

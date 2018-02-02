// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CROSTINI_CROSTINI_UTIL_H_
#define CHROME_BROWSER_CHROMEOS_CROSTINI_CROSTINI_UTIL_H_

namespace crostini {

// Whether project Crostini, i.e. running a Linux VM on Chrome OS is allowed per
// enterprise policy.
bool IsCrostiniAllowed();

}  // namespace crostini

#endif  // CHROME_BROWSER_CHROMEOS_CROSTINI_CROSTINI_UTIL_H_

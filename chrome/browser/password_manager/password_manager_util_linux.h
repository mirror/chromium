// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_MANAGER_UTIL_LINUX_H_
#define CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_MANAGER_UTIL_LINUX_H_

#include <limits>
#include <string>

#include "chrome/browser/password_manager/password_store_factory.h"

constexpr int kMaxPossibleTimeTValue = std::numeric_limits<int>::max();
extern const char kLibsecretAndGnomeAppString[];

// Generates a profile-specific app string based on profile_id_.
std::string GetProfileSpecificAppString(LocalProfileId id);


#endif  // CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_MANAGER_UTIL_LINUX_H_

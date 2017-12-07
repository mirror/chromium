// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEEDBACK_FEEDBACK_UTIL_CHROMEOS_H_
#define COMPONENTS_FEEDBACK_FEEDBACK_UTIL_CHROMEOS_H_

#include <string>

namespace feedback_util {

// Some system log entries are enormous and obscure, in which case we're better
// off displaying a different string explaining what the contents of these logs
// are rather than dumping a bunch of random hex into the feedback window.
bool GetDisplayValue(const std::string& key, std::string* to_display);

}  // namespace feedback_util

#endif  // COMPONENTS_FEEDBACK_FEEDBACK_UTIL_CHROMEOS_H_

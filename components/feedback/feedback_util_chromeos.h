// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEEDBACK_FEEDBACK_UTIL_CHROMEOS_H_
#define COMPONENTS_FEEDBACK_FEEDBACK_UTIL_CHROMEOS_H_

#include <map>
#include <memory>
#include <string>

#include <base/callback.h>

namespace feedback_util {

// Some system log entries are enormous and obscure, in which case we're better
// off displaying a different string explaining what the contents of these logs
// are rather than dumping a bunch of random hex into the feedback window.
// If |key| has a corresponding display value, this returns that display value.
// Otherwise, this returns nullptr.
std::unique_ptr<std::string> GetDisplayValue(const std::string& key);

// Asynchronously grab logs from debugd which would normally be too large or
// obscure to display to the user. These should be the logs for which display
// values were shown to the user in the system information dialog.
using GetSpecialLogsCallback = base::Callback<
    void(const std::map<std::string, std::string>&)>;
void GetSpecialLogs(const GetSpecialLogsCallback& callback);

}  // namespace feedback_util

#endif  // COMPONENTS_FEEDBACK_FEEDBACK_UTIL_CHROMEOS_H_

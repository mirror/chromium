// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_EVALUATE_CAPABILITY_H_
#define REMOTING_HOST_EVALUATE_CAPABILITY_H_

#include <string>

namespace remoting {

#if defined(UNIT_TEST)
// Executes EvaluateCapabilityForkedly() against host binary instead of current
// binary.
int EvaluateCapabilityAgainstHostBinary(const std::string& type,
                                        std::string* output = nullptr);
#endif

// Evaluates the host capability in current process. Note, this function may
// execute some experimental features and crash the process.
int EvaluateCapabilityLocally(const std::string& type);

// Evaluates the host capability in a different process and returns its exit
// code. If |output| is provided, it will be set to the stdout of the process.
// If the process failed to be started, though usually this should not happend,
// it returns TERMINATION_STATUS_LAUNCH_FAILED.
int EvaluateCapabilityForkedly(const std::string& type,
                               std::string* output = nullptr);

}  // namespace remoting

#endif  // REMOTING_HOST_EVALUATE_CAPABILITY_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_PROFILING_MEMLOG_SENDER_H_
#define CHROME_COMMON_PROFILING_MEMLOG_SENDER_H_

#include <string>

namespace base {

class CommandLine;

}  // namespace base

namespace profiling {

void InitMemlogSenderIfNecessary(const base::CommandLine& cmdline);

void StartMemlogSender(const std::string& pipe_id);

// Tells the profiling process to try to connect to the profiling control
// channel. This must be done after the browser is ready to accept such a
// connection.
void StartProfilingMojo();

}  // namespace profiling

#endif  // CHROME_COMMON_PROFILING_MEMLOG_SENDER_H_

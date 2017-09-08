// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/current_process_stats_agent.h"

namespace remoting {

CurrentProcessStatsAgent::CurrentProcessStatsAgent(
    const std::string& process_name)
    : ProcessMetrics(process_name, base::Process::Current()) {}

CurrentProcessStatsAgent::~CurrentProcessStatsAgent() = default;

}  // namespace remoting

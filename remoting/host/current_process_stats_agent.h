// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_CURRENT_PROCESS_STATS_AGENT_H_
#define REMOTING_HOST_CURRENT_PROCESS_STATS_AGENT_H_

#include "remoting/host/process_metrics.h"

namespace remoting {

// A component to report statistic data of the current process.
class CurrentProcessStatsAgent final : public ProcessMetrics {
 public:
  explicit CurrentProcessStatsAgent(const std::string& process_name);
  ~CurrentProcessStatsAgent() override;
};

}  // namespace remoting

#endif  // REMOTING_HOST_CURRENT_PROCESS_STATS_AGENT_H_

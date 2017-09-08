// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_PROCESS_METRICS_H_
#define REMOTING_HOST_PROCESS_METRICS_H_

#include <memory>
#include <string>

#include "base/process/process.h"
#include "remoting/host/process_stats_agent.h"
#include "remoting/proto/process_stats.pb.h"

namespace base {
class ProcessMetrics;
}  // namespace base

namespace remoting {

// A wrapper of base::ProcessMetrics, but provides a better lifetime management
// of base::Process.
// TODO(zijiehe): ProcessStatsAgent interface should be removed.
class ProcessMetrics : public ProcessStatsAgent {
 public:
  // Creates a ProcessMetrics to track the resource usage of |process|.
  ProcessMetrics(const std::string& name, base::Process&& process);
  ~ProcessMetrics() override;

  // Checks whether the |process_| represents a valid process and |metrics_| is
  // implemented on the platform.
  bool IsValid() const;

  // Checks whether |process_| is still running. This may not be accurate, the
  // OS may reuse the process id.
  bool IsRunning() const;

  protocol::ProcessResourceUsage GetResourceUsage() override;

  // Kills the |process_| immediate, this is for tests only.
  bool TerminateForTesting();

 private:
  const std::string name_;
  base::Process process_;
  const std::unique_ptr<base::ProcessMetrics> metrics_;
};

}  // namespace remoting

#endif  // REMOTING_HOST_PROCESS_METRICS_H_

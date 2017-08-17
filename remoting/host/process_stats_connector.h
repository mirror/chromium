// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_PROCESS_STATS_CONNECTOR_H_
#define REMOTING_HOST_PROCESS_STATS_CONNECTOR_H_

#include <memory>

namespace remoting {

class ProcessStatsAgent;

// The interface to start and return a ProcessStatsAgent. Typically it should be
// an IpcProcessStatsAgent.
class ProcessStatsConnector {
 public:
  ProcessStatsConnector() = default;
  virtual ~ProcessStatsConnector() = default;

  // Creates and starts a ProcessStatsAgent. This function may return a nullptr.
  virtual std::unique_ptr<ProcessStatsAgent> StartProcessStatsAgent() = 0;
};

}  // namespace remoting

#endif  // REMOTING_HOST_PROCESS_STATS_CONNECTOR_H_

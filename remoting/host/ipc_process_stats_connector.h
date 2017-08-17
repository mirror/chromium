// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_IPC_PROCESS_STATS_CONNECTOR_H_
#define REMOTING_HOST_IPC_PROCESS_STATS_CONNECTOR_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "remoting/host/process_stats_connector.h"

namespace IPC {
class Sender;
}  // namespace IPC

namespace remoting {

namespace protocol {
class ProcessResourceUsage;
}  // namespace protocol

class IpcProcessStatsAgent;
class ProcessStatsAgent;

class IpcProcessStatsConnector : public ProcessStatsConnector {
 public:
  IpcProcessStatsConnector();
  ~IpcProcessStatsConnector() override;

  // ProcessStatsConnector implementation.
  std::unique_ptr<ProcessStatsAgent> StartProcessStatsAgent() override;

  // |sender| must outlive the ProcessStatsAgent returned by
  // StartProcessStatsAgent(). But *|sender| does not.
  void set_ipc_sender(std::unique_ptr<IPC::Sender>* sender);

  // Extracts the first ProcessResourceUsage from |usage| and forwards to
  // |agent_| if it's alive.
  void OnProcessStats(const protocol::AggregatedProcessResourceUsage& usage);

 private:
  std::unique_ptr<IPC::Sender>* sender_;
  base::WeakPtr<IpcProcessStatsAgent> agent_;
};

}  // namespace remoting

#endif  // REMOTING_HOST_IPC_PROCESS_STATS_CONNECTOR_H_

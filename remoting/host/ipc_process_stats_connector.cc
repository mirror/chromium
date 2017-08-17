// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/ipc_process_stats_connector.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "remoting/base/constants.h"
#include "remoting/host/ipc_process_stats_agent.h"

namespace remoting {

IpcProcessStatsConnector::IpcProcessStatsConnector() = default;
IpcProcessStatsConnector::~IpcProcessStatsConnector() = default;

std::unique_ptr<ProcessStatsAgent>
IpcProcessStatsConnector::StartProcessStatsAgent() {
  DCHECK(sender_);
  DCHECK(!agent_);
  auto agent = base::MakeUnique<IpcProcessStatsAgent>(
      sender_, kDefaultProcessStatsInterval);
  agent_ = agent->GetWeakPtr();
  return agent;
}

void IpcProcessStatsConnector::set_ipc_sender(
    std::unique_ptr<IPC::Sender>* sender) {
  DCHECK(sender);
  DCHEKC(!sender_);
  sender_ = sender;
}

void IpcProcessStatsConnector::OnProcessStats(
    const protocol::AggregatedProcessResourceUsage& usage) {
  DCHECK_EQ(usage.usages_size(), 1);
  // If a message is received before stopping but is processed after stopping,
  // |agent_| may be invalided already.
  if (agent_ && usage.usages_size() > 0) {
    agent_->OnProcessStats(usage.usages(0));
  }
}

}  // namespace remoting

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/ipc_process_stats_agent.h"

#include "base/logging.h"
#include "ipc/ipc_sender.h"
#include "remoting/host/chromoting_messages.h"

namespace remoting {

IpcProcessStatsAgent::IpcProcessStatsAgent(IPC::Sender* sender,
                                           base::TimeDelta interval)
    : raw_sender_(sender),
      sender_ptr_(nullptr) {
  DCHECK(sender_);
  Start();
}

IpcProcessStatsAgent::IpcProcessStatsAgent(std::unique_ptr<IPC::Sender>* sender,
                                           base::TimeDelta interval)
    : raw_sender_(nullptr),
      sender_ptr_(sender) {
  DCHECK(sender_ptr_);
  Start();
}

IpcProcessStatsAgent::~IpcProcessStatsAgent() {
  Stop();
}

base::WeakPtr<IpcProcessStatsAgent> IpcProcessStatsAgent::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void IpcProcessStatsAgent::Start() {
  if (!sender()->Send(
      new ChromotingNetworkToAnyMsg_StartProcessStatsReport(interval))) {
    LOG(WARNING) << "Failed to send StartProcessStatsReport message.";
  }
}

void IpcProcessStatsAgent::Stop() {
  if (!sender()) {
    LOG(WARNING) << "IPC::Sender has been detached before the destruction of "
                    "IpcProcessStatsAgent.";
    return;
  }
  if (!sender()->Send(
      new ChromotingNetworkToAnyMsg_StopProcessStatsReport())) {
    LOG(WARNING) << "Failed to send StopProcessStatsReport message.";
  }
}

IPC::Sender* IpcProcessStatsAgent::sender() const {
  if (raw_sender_) {
    return raw_sender_;
  }
  return sender_ptr_->get();
}

}  // namespace remoting

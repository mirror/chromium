// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/process_stats_multi_forwarder.h"

#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "remoting/host/forward_process_stats_agent.h"
#include "remoting/host/process_stats_agent.h"

namespace remoting {

ProcessStatsMultiForwarder::ProcessStatsAgentWrapper::ProcessStatsAgentWrapper(
    ProcessStatsMultiForwarder* forwarder,
    std::unique_ptr<ForwardProcessStatsAgent> agent)
    : forwarder_(forwarder),
      agent_(std::move(agent)) {
  DCHECK(forwarder_);
  DCHECK(agent_);

  forwarder_->Register(this);
}

ProcessStatsMultiForwarder::
ProcessStatsAgentWrapper::~ProcessStatsAgentWrapper() {
  forwarder_->Unregister(this);
}

void ProcessStatsMultiForwarder::ProcessStatsAgentWrapper::OnProcessStats(
    const protocol::ProcessResourceUsage& usage) {
  agent_->OnProcessStats(usage);
}

ProcessStatsMultiForwarder::ProcessStatsMultiForwarder() {
  DETACH_FROM_THREAD(thread_checker_);
}

ProcessStatsMultiForwarder::~ProcessStatsMultiForwarder() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(agents_.empty());
}

void ProcessStatsMultiForwarder::Register(ProcessStatsAgentWrapper* wrapper) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(wrapper);
  for (size_t i = 0; i < forwarder_->agents_.size(); i++) {
    if (!agents_[i]) {
      agents_[i] = wrapper;
      return;
    }
  }
  agents_.push_back(wrapper);
}

void ProcessStatsMultiForwarder::Unregister(ProcessStatsAgentWrapper* wrapper) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(wrapper);
  for (size_t i = 0; i < forwarder_->agents_.size(); i++) {
    if (forwarder_->agents_[i] == this) {
      forwarder_->agents_[i] = nullptr;
      return;
    }
  }
  NOTREACHED();
}

std::unique_ptr<ProcessStatsAgent>
ProcessStatsMultiForwarder::StartProcessStatsAgent() {
  return base::MakeUnique<ProcessStatsAgentWrapper>(this, CreateStatsAgent());
}

void ProcessStatsMultiForwarder::OnProcessStats(
    const protocol::ProcessResourceUsage& usage) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  for (auto* agent : agents_) {
    if (agent) {
      agent->OnProcessStats(usage);
    }
  }
}

}  // namespace remoting

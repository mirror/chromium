// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/process_stats_message_handler.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "remoting/base/constants.h"
#include "remoting/host/desktop_environment.h"

namespace remoting {

ProcessStatsMessageHandler::ProcessStatsMessageHandler(
    const std::string& name,
    std::unique_ptr<protocol::MessagePipe> pipe,
    DesktopEnvironment* desktop_environment)
    : protocol::NamedMessagePipeHandler(name, std::move(pipe)),
      desktop_environment_(desktop_environment),
      caller_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_ptr_factory_(this) {}

ProcessStatsMessageHandler::~ProcessStatsMessageHandler() {
  // OnDisconnecting() should be executed before destructor.
  DCHECK(!timer_.IsRunning());
  AssertThreadAffinity();
}

void ProcessStatsMessageHandler::OnConnected() {
  AssertThreadAffinity();
  timer_.Start(FROM_HERE,
               kDefaultProcessStatsInterval,
               this,
               &ProcessStatsMessageHandler::OnRequestUsage);
}

void ProcessStatsMessageHandler::OnDisconnecting() {
  AssertThreadAffinity();
  timer_.Stop();
}

void ProcessStatsMessageHandler::OnRequestUsage() {
  AssertThreadAffinity();
  desktop_environment_->GetHostResourceUsage(
      base::Bind(&ProcessStatsMessageHandler::OnReceiveUsage,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ProcessStatsMessageHandler::OnReceiveUsage(
    const protocol::AggregatedProcessResourceUsage& usage) {
  if (!caller_task_runner_->BelongsToCurrentThread()) {
    caller_task_runner_->PostTask(FROM_HERE, base::Bind(
        &ProcessStatsMessageHandler::OnReceiveUsage,
        weak_ptr_factory_.GetWeakPtr(),
        usage));
    return;
  }

  AssertThreadAffinity();
  if (connected()) {
    Send(usage, base::Bind(&base::DoNothing));
  }
}

void ProcessStatsMessageHandler::AssertThreadAffinity() const {
  DCHECK(caller_task_runner_->BelongsToCurrentThread());
}

}  // namespace remoting

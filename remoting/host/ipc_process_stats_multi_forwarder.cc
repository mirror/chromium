// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/ipc_process_stats_multi_forwarder.h"

#include "base/memory/ptr_util.h"
#include "remoting/base/constants.h"
#include "remoting/host/ipc_process_stats_agent.h"

namespace remoting {

IpcProcessStatsMultiForwarder::IpcProcessStatsMultiForwarder(
    IPC::Sender* sender)
    : sender_(sender) {}

IpcProcessStatsMultiForwarder::~IpcProcessStatsMultiForwarder() = default;

std::unique_ptr<ForwardProcessStatsAgent>
IpcProcessStatsMultiForwarder::CreateStatsAgent() {
  return base::MakeUnique<IpcProcessStatsAgent>(
      sender_, kDefaultProcessStatsInterval);
}

}  // namespace remoting

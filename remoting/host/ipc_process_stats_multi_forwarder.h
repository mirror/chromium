// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_IPC_PROCESS_STATS_MULTI_FORWARDER_H_
#define REMOTING_HOST_IPC_PROCESS_STATS_MULTI_FORWARDER_H_

#include "remoting/host/process_stats_multi_forwarder.h"

namespace IPC {
class Sender;
}  // namespace IPC

namespace remoting {

class IpcProcessStatsMultiForwarder final : public ProcessStatsMultiForwarder {
 public:
  // |sender| needs to outlive this instance.
  IpcProcessStatsMultiForwarder(IPC::Sender* sender);
  ~IpcProcessStatsMultiForwarder() override;

 protected:
  std::unique_ptr<ForwardProcessStatsAgent> CreateStatsAgent() override;

 private:
  IPC::Sender* const sender_;
};

}  // namespace remoting

#endif  // REMOTING_HOST_IPC_PROCESS_STATS_MULTI_FORWARDER_H_

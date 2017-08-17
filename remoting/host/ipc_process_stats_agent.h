// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_IPC_PROCESS_STATS_AGENT_H_
#define REMOTING_HOST_IPC_PROCESS_STATS_AGENT_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "remoting/host/forward_process_stats_agent.h"

namespace IPC {
class Sender;
}  // namespace IPC

namespace remoting {

// ForwardProcessStatsAgent derived class to send StartProcessStatsReport
// message during constructor and StopProcessStatsReport message during
// destructor to the IPC::Sender.
// It accepts either IPC::Sender* or std::unique_ptr<IPC::Sender>*. For the
// later one, StopProcessStatsReport message won't be sent if the unique_ptr has
// been cleared.
// Do not use this class directly: always use IpcProcessStatsConnector or
// IpcProcessStatsMultiForwarder for a simpler lifetime management.
class IpcProcessStatsAgent final : public ForwardProcessStatsAgent {
 public:
  // |sender| needs to outlive this instance.
  IpcProcessStatsAgent(IPC::Sender* sender, base::TimeDelta interval);

  // |sender| needs to outlive this instance, but *|sender| does not.
  IpcProcessStatsAgent(std::unique_ptr<IPC::Sender>* sender,
                       base::TimeDelta interval);

  ~IpcProcessStatsAgent() override;

  base::WeakPtr<IpcProcessStatsAgent> GetWeakPtr();

 private:
  // Sends the StartProcessStatsReport message. Should only be called in one of
  // the constructor.
  void Start();

  // Sends the StopProcessStatsReport message. Should only be called in the
  // destructor.
  void Stop();

  // Returns either |raw_sender_| or *|sender_ptr_|.
  IPC::Sender* sender() const;

  IPC::Sender* raw_sender_;
  std::unique_ptr<IPC::Sender>* const sender_ptr_;

  base::WeakPtrFactory<IpcProcessStatsAgent> weak_factory_;
};

}  // namespace remoting

#endif  // REMOTING_HOST_IPC_PROCESS_STATS_AGENT_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_PROCESS_STATS_MESSAGE_HANDLER_H_
#define REMOTING_HOST_PROCESS_STATS_MESSAGE_HANDLER_H_

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/timer/timer.h"
#include "remoting/proto/process_stats.pb.h"
#include "remoting/protocol/named_message_pipe_handler.h"

namespace remoting {

class DesktopEnvironment;

constexpr char kProcessStatsDataChannelName[] = "process-stats";

// The NamedMessagePipeHandler to send process resource usgae statistic data to
// the client.
class ProcessStatsMessageHandler : public protocol::NamedMessagePipeHandler {
 public:
  // Caller needs to ensure the |desktop_environment| outlives this instance.
  ProcessStatsMessageHandler(const std::string& name,
                             std::unique_ptr<protocol::MessagePipe> pipe,
                             DesktopEnvironment* desktop_environment);
  ~ProcessStatsMessageHandler() override;

 private:
  // protocol::NamedMessagePipeHandler implementation.
  void OnConnected() override;
  void OnDisconnecting() override;

  // Sends a new host resource usage request to |desktop_environment_|.
  void OnRequestUsage();

  // The callback provided to |desktop_environment_| to receive the result of
  // the resource usage request. It forwards the |usage| through |pipe| to the
  // client.
  void OnReceiveUsage(const protocol::AggregatedProcessResourceUsage& usage);

  void AssertThreadAffinity() const;

  DesktopEnvironment* const desktop_environment_;
  // All methods should be executed in this thread, typically it's the network
  // thread.
  const scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner_;
  base::RepeatingTimer timer_;

  base::WeakPtrFactory<ProcessStatsMessageHandler> weak_ptr_factory_;
};

}  // namespace remoting

#endif  // REMOTING_HOST_PROCESS_STATS_MESSAGE_HANDLER_H_

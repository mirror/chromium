// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_PROCESS_STATS_MULTI_FORWARDER_H_
#define REMOTING_HOST_PROCESS_STATS_MULTI_FORWARDER_H_

#include <memory>
#include <vector>

#include "base/threading/thread_checker.h"

namespace remoting {

namespace protocol {

class ProcessResourceUsage;

}  // namespace protocol

class ForwardProcessStatsAgent;
class ProcessStatsAgent;

// Maintains a set of ForwardProcessStatsAgent instances and forwards
// OnProcessStats() to them.
class ProcessStatsMultiForwarder {
 public:
  ProcessStatsMultiForwarder();
  virtual ~ProcessStatsMultiForwarder();

  // Creates a new ProcessStatsAgent.
  std::unique_ptr<ProcessStatsAgent> StartProcessStatsAgent();

  // Forwards |usage| to all |agents_|.
  void OnProcessStats(const protocol::ProcessResourceUsage& usage) const;

 protected:
  virtual std::unique_ptr<ForwardProcessStatsAgent> CreateStatsAgent() = 0;

 private:
  class ProcessStatsAgentWrapper final : public ProcessStatsAgent {
   public:
	ProcessStatsAgentWrapper(ProcessStatsMultiForwarder* forwarder,
							 std::unique_ptr<ForwardProcessStatsAgent> agent);
	~ProcessStatsAgentWrapper() override;

    void OnProcessStats(const protocol::ProcessResourceUsage& usage);

   private:
	ProcessStatsMultiForwarder* const forwarder_;
	const std::unique_ptr<ForwardProcessStatsAgent> agent_;
  };

  friend ProcessStatsAgentWrapper::ProcessStatsAgentWrapper;
  friend ProcessStatsAgentWrapper::~ProcessStatsAgentWrapper;

  void Register(ProcessStatsAgentWrapper* wrapper);
  void Unregister(ProcessStatsAgentWrapper* wrapper);

  std::vector<ProcessStatsAgentWrapper*> agents_;

  THREAD_CHECKER(thread_checker_);
};

}  // namespace remoting

#endif  // REMOTING_HOST_PROCESS_STATS_MULTI_FORWARDER_H_

// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _CONTENT_BROWSER_TRACING_ANDROID_TRACING_AGENT_H_
#define _CONTENT_BROWSER_TRACING_ANDROID_TRACING_AGENT_H_

#include <memory>
#include <string>

#include "services/resource_coordinator/public/cpp/tracing/default_agent.h"

namespace service_manager {
class Connector;
}  // namespace service_manager

namespace content {

// This agent does not send any trace event to the coordinator. It writes to
// trace_marker.
class AndroidTracingAgent : public tracing::DefaultAgent {
 public:
  AndroidTracingAgent(service_manager::Connector* connector);

 private:
  friend std::default_delete<AndroidTracingAgent>;

  ~AndroidTracingAgent() override;

  // mojom::Agent. Called by Mojo internals on the UI thread.
  void StartTracing(const std::string& config,
                    base::TimeTicks coordinator_time,
                    const Agent::StartTracingCallback& callback) override;
  void StopAndFlush(tracing::mojom::RecorderPtr recorder) override;

  DISALLOW_COPY_AND_ASSIGN(AndroidTracingAgent);
};

}  // namespace content
#endif  // _CONTENT_BROWSER_TRACING_ANDROID_TRACING_AGENT_H_

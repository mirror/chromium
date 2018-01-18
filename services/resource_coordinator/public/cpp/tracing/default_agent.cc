// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/tracing/default_agent.h"

#include "services/resource_coordinator/public/interfaces/service_constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace tracing {

DefaultAgent::DefaultAgent() : binding_(this) {}

DefaultAgent::~DefaultAgent() = default;

void DefaultAgent::Register(service_manager::Connector* connector,
                            const std::string& label,
                            mojom::TraceDataType type,
                            bool supports_explicit_clock_sync) {
  // Connecto to the agent registry interface.
  tracing::mojom::AgentRegistryPtr agent_registry;
  connector->BindInterface(resource_coordinator::mojom::kServiceName,
                           &agent_registry);

  // Register this agent.
  tracing::mojom::AgentPtr agent;
  binding_.Bind(mojo::MakeRequest(&agent));
  agent_registry->RegisterAgent(std::move(agent), label, type,
                                supports_explicit_clock_sync);
}

void DefaultAgent::StartTracing(const std::string& config,
                                base::TimeTicks coordinator_time,
                                const Agent::StartTracingCallback& callback) {
  callback.Run(true /* success */);
}

void DefaultAgent::StopAndFlush(tracing::mojom::RecorderPtr recorder) {}

void DefaultAgent::RequestClockSyncMarker(
    const std::string& sync_id,
    const Agent::RequestClockSyncMarkerCallback& callback) {
  NOTREACHED() << "This agent does not support explicit clock sync";
}

void DefaultAgent::GetCategories(const Agent::GetCategoriesCallback& callback) {
  callback.Run("" /* categories */);
}

void DefaultAgent::RequestBufferStatus(
    const Agent::RequestBufferStatusCallback& callback) {
  callback.Run(0 /* capacity */, 0 /* count */);
}

}  // namespace tracing

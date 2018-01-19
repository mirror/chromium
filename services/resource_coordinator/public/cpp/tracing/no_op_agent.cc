// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/tracing/no_op_agent.h"

#include "services/resource_coordinator/public/interfaces/service_constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace tracing {

NoOpAgent::NoOpAgent() : binding_(this) {}

NoOpAgent::~NoOpAgent() = default;

void NoOpAgent::Register(service_manager::Connector* connector,
                         const std::string& label,
                         mojom::TraceDataType type,
                         bool supports_explicit_clock_sync) {
  tracing::mojom::AgentRegistryPtr agent_registry;
  connector->BindInterface(resource_coordinator::mojom::kServiceName,
                           &agent_registry);

  tracing::mojom::AgentPtr agent;
  binding_.Bind(mojo::MakeRequest(&agent));
  agent_registry->RegisterAgent(std::move(agent), label, type,
                                supports_explicit_clock_sync);
}

void NoOpAgent::StartTracing(const std::string& config,
                             base::TimeTicks coordinator_time,
                             const Agent::StartTracingCallback& callback) {
  callback.Run(true /* success */);
}

void NoOpAgent::StopAndFlush(tracing::mojom::RecorderPtr recorder) {}

void NoOpAgent::RequestClockSyncMarker(
    const std::string& sync_id,
    const Agent::RequestClockSyncMarkerCallback& callback) {
  NOTREACHED() << "The agent claims to support explicit clock sync but does "
               << "not override NoOpAgent::RequestClockSyncMarker()";
}

void NoOpAgent::GetCategories(const Agent::GetCategoriesCallback& callback) {
  callback.Run("" /* categories */);
}

void NoOpAgent::RequestBufferStatus(
    const Agent::RequestBufferStatusCallback& callback) {
  callback.Run(0 /* capacity */, 0 /* count */);
}

}  // namespace tracing

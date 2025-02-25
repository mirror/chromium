// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_TRACING_BASE_AGENT_H_
#define SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_TRACING_BASE_AGENT_H_

#include <string>

#include "mojo/public/cpp/bindings/binding.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_export.h"
#include "services/resource_coordinator/public/interfaces/tracing/tracing.mojom.h"

namespace service_manager {
class Connector;
}  // namespace service_manager

// This class is a minimal implementation of mojom::Agent to reduce boilerplate
// code in tracing agents. A tracing agent can inherit from this class and only
// override methods that actually do something, in most cases only StartTracing
// and StopAndFlush.
namespace tracing {
class SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_EXPORT BaseAgent
    : public mojom::Agent {
 protected:
  BaseAgent(service_manager::Connector* connector,
            const std::string& label,
            mojom::TraceDataType type,
            bool supports_explicit_clock_sync);
  ~BaseAgent() override;

 private:
  // tracing::mojom::Agent:
  void StartTracing(const std::string& config,
                    base::TimeTicks coordinator_time,
                    const Agent::StartTracingCallback& callback) override;
  void StopAndFlush(tracing::mojom::RecorderPtr recorder) override;
  void RequestClockSyncMarker(
      const std::string& sync_id,
      const Agent::RequestClockSyncMarkerCallback& callback) override;
  void GetCategories(const Agent::GetCategoriesCallback& callback) override;
  void RequestBufferStatus(
      const Agent::RequestBufferStatusCallback& callback) override;

  mojo::Binding<tracing::mojom::Agent> binding_;

  DISALLOW_COPY_AND_ASSIGN(BaseAgent);
};

}  // namespace tracing

#endif  // SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_TRACING_BASE_AGENT_H_

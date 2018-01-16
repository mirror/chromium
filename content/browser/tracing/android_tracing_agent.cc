// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/tracing/android_tracing_agent.h"

#include "base/trace_event/trace_log.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

AndroidTracingAgent::AndroidTracingAgent(
    service_manager::Connector* connector) {
  Register(connector, "androidTraceEvents",
           tracing::mojom::TraceDataType::STRING,
           true /* supports_explicit_clock_sync */);
}

AndroidTracingAgent::~AndroidTracingAgent() = default;

void AndroidTracingAgent::StartTracing(
    const std::string& config,
    base::TimeTicks coordinator_time,
    const Agent::StartTracingCallback& callback) {
  base::trace_event::TraceLog::GetInstance()->AddClockSyncMetadataEvent();
  callback.Run(true /* success */);
}

void AndroidTracingAgent::StopAndFlush(tracing::mojom::RecorderPtr recorder) {
  base::trace_event::TraceLog::GetInstance()->AddClockSyncMetadataEvent();
}

}  // namespace content

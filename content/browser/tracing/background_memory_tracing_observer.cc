// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/tracing/background_memory_tracing_observer.h"

#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/memory_dump_request_args.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/service_names.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

// static
BackgroundMemoryTracingObserver*
BackgroundMemoryTracingObserver::GetInstance() {
  static auto* instance = new BackgroundMemoryTracingObserver();
  return instance;
}

BackgroundMemoryTracingObserver::BackgroundMemoryTracingObserver() {}
BackgroundMemoryTracingObserver::~BackgroundMemoryTracingObserver() {}

void BackgroundMemoryTracingObserver::OnScenarioActivated(
    const BackgroundTracingConfigImpl* config) {}

void BackgroundMemoryTracingObserver::OnTracingEnabled(
    BackgroundTracingConfigImpl::CategoryPreset preset) {
  if (preset !=
      BackgroundTracingConfigImpl::CategoryPreset::BENCHMARK_MEMORY_LIGHT)
    return;

  service_manager::Connector* connector =
      content::ServiceManagerConnection::GetForProcess()->GetConnector();
  connector->BindInterface(content::mojom::kBrowserServiceName,
                           mojo::MakeRequest(&memory_instrumentation_));
  base::trace_event::MemoryDumpRequestArgs args = {
      0 /* guid TODO */,
      base::trace_event::MemoryDumpType::EXPLICITLY_TRIGGERED,
      base::trace_event::MemoryDumpLevelOfDetail::BACKGROUND};
  memory_instrumentation_->RequestGlobalMemoryDump(
      args, memory_instrumentation::mojom::Coordinator::
                RequestGlobalMemoryDumpCallback());
}

}  // namespace content

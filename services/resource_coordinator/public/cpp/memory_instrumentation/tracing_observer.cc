// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/memory_instrumentation/tracing_observer.h"

#include "base/trace_event/memory_dump_manager.h"

namespace memory_instrumentation {

namespace {

bool IsMemoryInfraTracingEnabled() {
  bool enabled;
  TRACE_EVENT_CATEGORY_GROUP_ENABLED(
      base::trace_event::MemoryDumpManager::kTraceCategory, &enabled);
  return enabled;
}

}  // namespace

TracingObserver::TracingObserver(
    base::trace_event::TraceLog* trace_log,
    base::trace_event::MemoryDumpManager* memory_dump_manager)
    : memory_dump_manager_(memory_dump_manager), trace_log_(trace_log) {
  // If tracing was enabled before initializing MemoryDumpManager, we missed the
  // OnTraceLogEnabled() event. Synthesize it so we can late-join the party.
  // IsEnabled is called before adding observer to avoid calling
  // OnTraceLogEnabled twice.
  bool is_tracing_already_enabled = trace_log_->IsEnabled();
  trace_log_->AddEnabledStateObserver(this);
  if (is_tracing_already_enabled)
    OnTraceLogEnabled();
}

TracingObserver::~TracingObserver() {
  trace_log_->RemoveEnabledStateObserver(this);
}

void TracingObserver::OnTraceLogEnabled() {
  if (!IsMemoryInfraTracingEnabled())
    return;

  // Initialize the TraceLog for the current thread. This is to avoids that the
  // TraceLog memory dump provider is registered lazily during the MDM
  // SetupForTracing().
  base::trace_event::TraceLog::GetInstance()
      ->InitializeThreadLocalEventBufferIfSupported();

  memory_dump_manager_->SetupForTracing(
      trace_log_->GetCurrentTraceConfig().memory_dump_config());
}

void TracingObserver::OnTraceLogDisabled() {
  memory_dump_manager_->TeardownForTracing();
}

}  // namespace memory_instrumentation

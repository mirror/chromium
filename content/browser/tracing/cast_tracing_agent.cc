// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/tracing/cast_tracing_agent.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/trace_event/trace_config.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/resource_coordinator/public/interfaces/service_constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

CastTracingAgent::CastTracingAgent(service_manager::Connector* connector)
    : binding_(this) {
  // Connecto to the agent registry interface.
  tracing::mojom::AgentRegistryPtr agent_registry;
  connector->BindInterface(resource_coordinator::mojom::kServiceName,
                           &agent_registry);

  // Register this agent.
  tracing::mojom::AgentPtr agent;
  binding_.Bind(mojo::MakeRequest(&agent));
  static const char kFtraceLabel[] = "systemTraceEvents";
  agent_registry->RegisterAgent(std::move(agent), kFtraceLabel,
                                tracing::mojom::TraceDataType::STRING,
                                false /* supports_explicit_clock_sync */);
}

CastTracingAgent::~CastTracingAgent() = default;

// tracing::mojom::Agent. Called by Mojo internals on the UI thread.
void CastTracingAgent::StartTracing(
    const std::string& config,
    base::TimeTicks coordinator_time,
    const Agent::StartTracingCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  base::trace_event::TraceConfig trace_config(config);

  if (!trace_config.IsSystraceEnabled()) {
    callback.Run(false /* success */);
    return;
  }

  start_tracing_callback_ = callback;

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindRepeating(&CastTracingAgent::StartTracingOnIO,
                          base::Unretained(this)));
}

void CastTracingAgent::StopAndFlush(tracing::mojom::RecorderPtr recorder) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  recorder_ = std::move(recorder);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindRepeating(&CastTracingAgent::StopAndFlushOnIO,
                          base::Unretained(this)));
}

void CastTracingAgent::RequestClockSyncMarker(
    const std::string& sync_id,
    const Agent::RequestClockSyncMarkerCallback& callback) {
  NOTREACHED();
}

void CastTracingAgent::GetCategories(
    const Agent::GetCategoriesCallback& callback) {
  callback.Run("" /* categories */);
}

void CastTracingAgent::RequestBufferStatus(
    const Agent::RequestBufferStatusCallback& callback) {
  callback.Run(0 /* capacity */, 0 /* count */);
}

void CastTracingAgent::StartTracingOnIO() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  cast_system_trace_ = std::make_unique<chromecast::SystemTracer>();

  // TODO(spang): Grab categories from trace config.
  cast_system_trace_->StartTracing(
      "gfx,input,power,sched,workq",
      base::BindOnce(&CastTracingAgent::FinishStartOnIO,
                     base::Unretained(this)));
}

void CastTracingAgent::FinishStartOnIO(
    chromecast::SystemTracer::Status status) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::BindOnce(&CastTracingAgent::FinishStart,
                                         base::Unretained(this), status));
}

void CastTracingAgent::FinishStart(chromecast::SystemTracer::Status status) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  start_tracing_callback_.Run(status == chromecast::SystemTracer::Status::OK);
}

void CastTracingAgent::StopAndFlushOnIO() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  cast_system_trace_->StopTracing(base::BindRepeating(
      &CastTracingAgent::HandleTraceDataOnIO, base::Unretained(this)));
}

void CastTracingAgent::HandleTraceDataOnIO(
    chromecast::SystemTracer::Status status,
    std::string trace_data) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&CastTracingAgent::HandleTraceData, base::Unretained(this),
                     status, std::move(trace_data)));
}

void CastTracingAgent::HandleTraceData(chromecast::SystemTracer::Status status,
                                       std::string trace_data) {
  if (recorder_ && status != chromecast::SystemTracer::Status::FAIL)
    recorder_->AddChunk(std::move(trace_data));
  if (status != chromecast::SystemTracer::Status::KEEP_GOING)
    recorder_.reset();
}

}  // namespace content

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/tracing/power_tracing_agent.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/memory/singleton.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event_impl.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/resource_coordinator/public/interfaces/tracing/tracing.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "tools/battor_agent/battor_finder.h"

namespace content {

namespace {

const char kPowerTracingAgentName[] = "battor";
const char kPowerTraceLabel[] = "powerTraceAsString";

}  // namespace

// static
PowerTracingAgent* PowerTracingAgent::GetInstance() {
  return base::Singleton<PowerTracingAgent>::get();
}

PowerTracingAgent::PowerTracingAgent(service_manager::Connector* connector)
    : binding_(this) {
  // Connecto to the agent registry interface.
  tracing::mojom::AgentRegistryPtr agent_registry;
  connector->BindInterface("content_browser", &agent_registry);

  // Register this agent.
  tracing::mojom::AgentPtr agent;
  binding_.Bind(mojo::MakeRequest(&agent));
  agent_registry->RegisterAgent(std::move(agent), kPowerTraceLabel,
                                tracing::mojom::TraceDataType::STRING,
                                true /* supports_explicit_clock_sync */);
}

PowerTracingAgent::PowerTracingAgent() : binding_(this) {}
PowerTracingAgent::~PowerTracingAgent() {}

std::string PowerTracingAgent::GetTracingAgentName() {
  return kPowerTracingAgentName;
}

std::string PowerTracingAgent::GetTraceEventLabel() {
  return kPowerTraceLabel;
}

void PowerTracingAgent::StartAgentTracing(
    const base::trace_event::TraceConfig& trace_config,
    const StartAgentTracingCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  StartTracing("", base::BindRepeating(callback, GetTracingAgentName()));
}

void PowerTracingAgent::StartTracing(
    const std::string& config,
    const Agent::StartTracingCallback& callback) {
  BrowserThread::PostTask(BrowserThread::FILE, FROM_HERE,
                          base::Bind(&PowerTracingAgent::FindBattOrOnFileThread,
                                     base::Unretained(this), callback));
}

void PowerTracingAgent::FindBattOrOnFileThread(
    const Agent::StartTracingCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);

  std::string path = battor::BattOrFinder::FindBattOr();
  if (path.empty()) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::Bind(callback, false /* success */));
    return;
  }

  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(&PowerTracingAgent::StartTracingOnIOThread,
                                     base::Unretained(this), path, callback));
}

void PowerTracingAgent::StartTracingOnIOThread(
    const std::string& path,
    const Agent::StartTracingCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  battor_agent_.reset(new battor::BattOrAgent(
      path, this, BrowserThread::GetTaskRunnerForThread(BrowserThread::UI)));

  start_tracing_callback_ = callback;
  battor_agent_->StartTracing();
}

void PowerTracingAgent::OnStartTracingComplete(battor::BattOrError error) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  bool success = (error == battor::BATTOR_ERROR_NONE);
  if (!success)
    battor_agent_.reset();

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(start_tracing_callback_, success));
  start_tracing_callback_.Reset();
}

void PowerTracingAgent::StopAndFlush(tracing::mojom::RecorderPtr recorder) {
  recorder_ = std::move(recorder);
  StopAgentTracing(base::BindRepeating(&PowerTracingAgent::RecorderProxy,
                                       base::Unretained(this)));
}

void PowerTracingAgent::RecorderProxy(
    const std::string& agent_name,
    const std::string& events_label,
    const scoped_refptr<base::RefCountedString>& events) {
  if (events && !events->data().empty())
    recorder_->AddChunk(events->data());
  recorder_.reset();
}

void PowerTracingAgent::StopAgentTracing(
    const StopAgentTracingCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(&PowerTracingAgent::StopAndFlushOnIOThread,
                                     base::Unretained(this), callback));
}

void PowerTracingAgent::StopAndFlushOnIOThread(
    const StopAgentTracingCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!battor_agent_) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(callback, GetTracingAgentName(), GetTraceEventLabel(),
                   nullptr /* events_str_ptr */));
    return;
  }

  stop_tracing_callback_ = callback;
  battor_agent_->StopTracing();
}

void PowerTracingAgent::OnStopTracingComplete(const std::string& trace,
                                              battor::BattOrError error) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  scoped_refptr<base::RefCountedString> result(new base::RefCountedString());
  if (error == battor::BATTOR_ERROR_NONE)
    result->data() = trace;

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(stop_tracing_callback_, GetTracingAgentName(),
                 GetTraceEventLabel(), result));
  stop_tracing_callback_.Reset();
  battor_agent_.reset();
}

void PowerTracingAgent::RecordClockSyncMarker(
    const std::string& sync_id,
    const RecordClockSyncMarkerCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(SupportsExplicitClockSync());

  RequestClockSyncMarker(sync_id, base::BindRepeating(callback, sync_id));
}

void PowerTracingAgent::RequestClockSyncMarker(
    const std::string& sync_id,
    const Agent::RequestClockSyncMarkerCallback& callback) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&PowerTracingAgent::RequestClockSyncMarkerOnIOThread,
                 base::Unretained(this), sync_id, callback));
}

void PowerTracingAgent::RequestClockSyncMarkerOnIOThread(
    const std::string& sync_id,
    const Agent::RequestClockSyncMarkerCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(battor_agent_);

  request_clock_sync_marker_callback_ = callback;
  request_clock_sync_marker_start_time_ = base::TimeTicks::Now();
  battor_agent_->RecordClockSyncMarker(sync_id);
}

void PowerTracingAgent::OnRecordClockSyncMarkerComplete(
    battor::BattOrError error) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  base::TimeTicks issue_start_ts = request_clock_sync_marker_start_time_;
  base::TimeTicks issue_end_ts = base::TimeTicks::Now();

  if (error != battor::BATTOR_ERROR_NONE)
    issue_start_ts = issue_end_ts = base::TimeTicks();

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(request_clock_sync_marker_callback_,
                                     issue_start_ts, issue_end_ts));

  request_clock_sync_marker_callback_.Reset();
  request_clock_sync_marker_start_time_ = base::TimeTicks();
}

bool PowerTracingAgent::SupportsExplicitClockSync() {
  return battor_agent_->SupportsExplicitClockSync();
}

void PowerTracingAgent::OnGetFirmwareGitHashComplete(
    const std::string& version, battor::BattOrError error) {
  return;
}

void PowerTracingAgent::GetCategories(
    const Agent::GetCategoriesCallback& callback) {
  callback.Run("");
}

void PowerTracingAgent::RequestBufferStatus(
    const Agent::RequestBufferStatusCallback& callback) {
  callback.Run(0, 0);
}

}  // namespace content

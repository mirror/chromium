// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/tracing/power_tracing_agent.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/lazy_instance.h"
#include "base/memory/singleton.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event_impl.h"
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

PowerTracingAgent::PowerTracingAgent()
    : task_runner_(base::CreateSequencedTaskRunnerWithTraits({})) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

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
  // TODO(charliea): StartAgentTracing for each of the tracing agents is called
  // on the UI thread and, to avoid doing real work on the UI thread, each agent
  // should immediately move work to another thread. A more sensible system
  // would be to call StartAgentTracing on a non-UI thread in the first place.
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&PowerTracingAgent::FindBattOr, base::Unretained(this),
                     base::BindOnce(&PowerTracingAgent::StartAgentTracingImpl,
                                    base::Unretained(this), callback)));
}

void PowerTracingAgent::FindBattOr(
    base::OnceCallback<void(const std::string& path)> callback) {
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), battor::BattOrFinder::FindBattOr()));
}

void PowerTracingAgent::StartAgentTracingImpl(
    const StartAgentTracingCallback& callback,
    const std::string& path) {
  DCHECK(sequence_checker_.CalledOnValidSequence());

  battor_agent_.reset(new battor::BattOrAgent(
      path, this, BrowserThread::GetTaskRunnerForThread(BrowserThread::UI)));

  start_tracing_callback_ = callback;
  battor_agent_->StartTracing();
}

void PowerTracingAgent::OnStartTracingComplete(battor::BattOrError error) {
  DCHECK(sequence_checker_.CalledOnValidSequence());

  bool success = (error == battor::BATTOR_ERROR_NONE);
  if (!success)
    battor_agent_.reset();

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(start_tracing_callback_, GetTracingAgentName(), success));
  start_tracing_callback_.Reset();
}

void PowerTracingAgent::StopAgentTracing(
    const StopAgentTracingCallback& callback) {
  // TODO(charliea): StopAgentTracing for each of the tracing agents is called
  // on the UI thread and, to avoid doing real work on the UI thread, each agent
  // should immediately move work to another thread. A more sensible system
  // would be to call StopAgentTracing on a non-UI thread in the first place.
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&PowerTracingAgent::StopAgentTracingImpl,
                                base::Unretained(this), callback));
}

void PowerTracingAgent::StopAgentTracingImpl(
    const StopAgentTracingCallback& callback) {
  DCHECK(sequence_checker_.CalledOnValidSequence());

  if (!battor_agent_) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(callback, GetTracingAgentName(), GetTraceEventLabel(),
                       nullptr /* events_str_ptr */));
    return;
  }

  stop_tracing_callback_ = callback;
  battor_agent_->StopTracing();
}

void PowerTracingAgent::OnStopTracingComplete(
    const battor::BattOrResults& results,
    battor::BattOrError error) {
  DCHECK(sequence_checker_.CalledOnValidSequence());

  scoped_refptr<base::RefCountedString> result(new base::RefCountedString());
  if (error == battor::BATTOR_ERROR_NONE)
    result->data() = results.ToString();

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(stop_tracing_callback_, GetTracingAgentName(),
                     GetTraceEventLabel(), result));
  stop_tracing_callback_.Reset();
  battor_agent_.reset();
}

void PowerTracingAgent::RecordClockSyncMarker(
    const std::string& sync_id,
    const RecordClockSyncMarkerCallback& callback) {
  // TODO(charliea): RecordClockSyncMarker for each of the tracing agents is
  // called on the UI thread and, to avoid doing real work on the UI thread,
  // each agent should immediately move work to another thread. A more sensible
  // system would be to call RecordClockSyncMarker on a non-UI thread in the
  // first place.
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(SupportsExplicitClockSync());

  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&PowerTracingAgent::RecordClockSyncMarkerImpl,
                                base::Unretained(this), callback, sync_id));
}

void PowerTracingAgent::RecordClockSyncMarkerImpl(
    const RecordClockSyncMarkerCallback& callback,
    const std::string& sync_id) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  DCHECK(battor_agent_);

  record_clock_sync_marker_sync_id_ = sync_id;
  record_clock_sync_marker_callback_ = callback;
  record_clock_sync_marker_start_time_ = base::TimeTicks::Now();
  battor_agent_->RecordClockSyncMarker(sync_id);
}

void PowerTracingAgent::OnRecordClockSyncMarkerComplete(
    battor::BattOrError error) {
  DCHECK(sequence_checker_.CalledOnValidSequence());

  base::TimeTicks issue_start_ts = record_clock_sync_marker_start_time_;
  base::TimeTicks issue_end_ts = base::TimeTicks::Now();

  if (error != battor::BATTOR_ERROR_NONE)
    issue_start_ts = issue_end_ts = base::TimeTicks();

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::BindOnce(record_clock_sync_marker_callback_,
                                         record_clock_sync_marker_sync_id_,
                                         issue_start_ts, issue_end_ts));

  record_clock_sync_marker_callback_.Reset();
  record_clock_sync_marker_sync_id_ = std::string();
  record_clock_sync_marker_start_time_ = base::TimeTicks();
}

bool PowerTracingAgent::SupportsExplicitClockSync() {
  return battor_agent_->SupportsExplicitClockSync();
}

void PowerTracingAgent::OnGetFirmwareGitHashComplete(
    const std::string& version,
    battor::BattOrError error) {
  return;
}

}  // namespace content

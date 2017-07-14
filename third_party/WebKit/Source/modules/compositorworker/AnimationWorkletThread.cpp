// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/compositorworker/AnimationWorkletThread.h"

#include "core/workers/GlobalScopeStartupData.h"
#include "modules/compositorworker/AnimationWorkletGlobalScope.h"
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "platform/wtf/PtrUtil.h"

namespace blink {

std::unique_ptr<AnimationWorkletThread> AnimationWorkletThread::Create(
    ThreadableLoadingContext* loading_context,
    WorkerReportingProxy& worker_reporting_proxy) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("animation-worklet"),
               "AnimationWorkletThread::create");
  DCHECK(IsMainThread());
  return WTF::WrapUnique(
      new AnimationWorkletThread(loading_context, worker_reporting_proxy));
}

AnimationWorkletThread::AnimationWorkletThread(
    ThreadableLoadingContext* loading_context,
    WorkerReportingProxy& worker_reporting_proxy)
    : AbstractAnimationWorkletThread(loading_context, worker_reporting_proxy) {}

AnimationWorkletThread::~AnimationWorkletThread() {}

WorkerOrWorkletGlobalScope* AnimationWorkletThread::CreateWorkerGlobalScope(
    std::unique_ptr<GlobalScopeStartupData> startup_data) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("animation-worklet"),
               "AnimationWorkletThread::createWorkerGlobalScope");

  RefPtr<SecurityOrigin> security_origin =
      SecurityOrigin::Create(startup_data->script_url);
  if (startup_data->starter_origin_privilege_data) {
    security_origin->TransferPrivilegesFrom(
        std::move(startup_data->starter_origin_privilege_data));
  }

  return AnimationWorkletGlobalScope::Create(
      startup_data->script_url, startup_data->user_agent,
      std::move(security_origin), this->GetIsolate(), this,
      startup_data->worker_clients);
}

}  // namespace blink

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/tracing/trace_message_filter.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "build/build_config.h"
#include "components/tracing/common/process_metrics_memory_dump_provider.h"
#include "components/tracing/common/tracing_messages.h"
#include "content/browser/tracing/background_tracing_manager_impl.h"
#include "content/common/child_process_host_impl.h"
#include "content/public/browser/browser_thread.h"

namespace content {

TraceMessageFilter::TraceMessageFilter(int child_process_id)
    : BrowserMessageFilter(TracingMsgStart), has_child_(false) {
  Send(new TracingMsg_SetTracingProcessId(
      ChildProcessHostImpl::ChildProcessUniqueIdToTracingProcessId(
          child_process_id)));
}

TraceMessageFilter::~TraceMessageFilter() {}

void TraceMessageFilter::OnChannelClosing() {
  if (has_child_) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&TraceMessageFilter::Unregister, base::RetainedRef(this)));
  }
}

bool TraceMessageFilter::OnMessageReceived(const IPC::Message& message) {
  // Always on IO thread (BrowserMessageFilter guarantee).
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(TraceMessageFilter, message)
    IPC_MESSAGE_HANDLER(TracingHostMsg_ChildSupportsTracing,
                        OnChildSupportsTracing)
    IPC_MESSAGE_HANDLER(TracingHostMsg_TriggerBackgroundTrace,
                        OnTriggerBackgroundTrace)
    IPC_MESSAGE_HANDLER(TracingHostMsg_AbortBackgroundTrace,
                        OnAbortBackgroundTrace)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void TraceMessageFilter::OnChildSupportsTracing() {
  has_child_ = true;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&TraceMessageFilter::Register, base::RetainedRef(this)));
}

void TraceMessageFilter::Register() {
  BackgroundTracingManagerImpl::GetInstance()->AddTraceMessageFilter(this);
#if defined(OS_LINUX)
  // On Linux the browser process dumps process metrics for child process due to
  // sandbox.
  tracing::ProcessMetricsMemoryDumpProvider::RegisterForProcess(peer_pid());
#endif
}

void TraceMessageFilter::Unregister() {
  BackgroundTracingManagerImpl::GetInstance()->RemoveTraceMessageFilter(this);
}

void TraceMessageFilter::OnTriggerBackgroundTrace(const std::string& name) {
  BackgroundTracingManagerImpl::GetInstance()->OnHistogramTrigger(name);
}

void TraceMessageFilter::OnAbortBackgroundTrace() {
  BackgroundTracingManagerImpl::GetInstance()->AbortScenario();
}

}  // namespace content

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/tracing/arc_tracing_agent_impl.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file.h"
#include "base/files/file_descriptor_watcher_posix.h"
#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/posix/unix_domain_socket_linux.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "cc/base/ring_buffer.h"
#include "content/public/browser/arc_tracing_agent.h"
#include "content/public/browser/browser_thread.h"
#include "services/resource_coordinator/public/interfaces/tracing/tracing.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

namespace {

ArcTracingAgentImpl* g_arc_tracing_agent_impl = nullptr;

// The maximum size used to store one trace event. The ad hoc trace format for
// atrace is 1024 bytes. Here we add additional size as we're using JSON and
// have additional data fields.
constexpr size_t kArcTraceMessageLength = 1024 + 512;

constexpr char kArcTracingAgentName[] = "arc";
constexpr char kArcTraceLabel[] = "ArcTraceEvents";

}  // namespace

// static
ArcTracingAgent* ArcTracingAgent::GetInstance() {
  return ArcTracingAgentImpl::GetInstance();
}

ArcTracingAgent::ArcTracingAgent() = default;

ArcTracingAgent::~ArcTracingAgent() = default;

// static
ArcTracingAgentImpl* ArcTracingAgentImpl::GetInstance() {
  DCHECK(g_arc_tracing_agent_impl);
  return g_arc_tracing_agent_impl;
}

ArcTracingAgentImpl::ArcTracingAgentImpl(service_manager::Connector* connector)
    : binding_(this), weak_ptr_factory_(this) {
  DCHECK(!g_arc_tracing_agent_impl);
  g_arc_tracing_agent_impl = this;
}

ArcTracingAgentImpl::~ArcTracingAgentImpl() {
  NOTREACHED();
}

std::string ArcTracingAgentImpl::GetTracingAgentName() {
  return kArcTracingAgentName;
}

std::string ArcTracingAgentImpl::GetTraceEventLabel() {
  return kArcTraceLabel;
}

void ArcTracingAgentImpl::StartAgentTracing(
    const base::trace_event::TraceConfig& trace_config,
    const StartAgentTracingCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  StartTracing(trace_config.ToString(),
               base::BindRepeating(callback, GetTracingAgentName()));
}

void ArcTracingAgentImpl::StopAgentTracing(
    const StopAgentTracingCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (is_stopping_) {
    DLOG(WARNING) << "Already working on stopping ArcTracingAgent.";
    return;
  }
  is_stopping_ = true;
  if (delegate_) {
    delegate_->StopTracing(base::Bind(&ArcTracingAgentImpl::OnArcTracingStopped,
                                      weak_ptr_factory_.GetWeakPtr(),
                                      callback));
  }
}

void ArcTracingAgentImpl::SetDelegate(Delegate* delegate) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  delegate_ = delegate;
}

void ArcTracingAgentImpl::OnArcTracingStopped(
    const StopAgentTracingCallback& callback,
    bool success) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!success) {
    DLOG(WARNING) << "Failed to stop ARC tracing.";
    callback.Run(GetTracingAgentName(), GetTraceEventLabel(),
                 new base::RefCountedString());
    is_stopping_ = false;
    return;
  }
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&ArcTracingReader::StopTracing, reader_.GetWeakPtr(),
                 base::Bind(&ArcTracingAgentImpl::OnTracingReaderStopped,
                            weak_ptr_factory_.GetWeakPtr(), callback)));
}

void ArcTracingAgentImpl::OnTracingReaderStopped(
    const StopAgentTracingCallback& callback,
    const scoped_refptr<base::RefCountedString>& data) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  callback.Run(GetTracingAgentName(), GetTraceEventLabel(), data);
  is_stopping_ = false;
}

void ArcTracingAgentImpl::StartTracing(
    const std::string& config,
    const Agent::StartTracingCallback& callback) {
  base::trace_event::TraceConfig trace_config(config);
  // delegate_ may be nullptr if ARC is not enabled on the system. In such
  // case, simply do nothing.
  bool success = (delegate_ != nullptr);

  base::ScopedFD write_fd, read_fd;
  success = success && CreateSocketPair(&read_fd, &write_fd);

  if (!success) {
    // Use PostTask as the convention of TracingAgent. The caller expects
    // callback to be called after this function returns.
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  base::Bind(callback, false));
    return;
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&ArcTracingReader::StartTracing, reader_.GetWeakPtr(),
                 base::Passed(&read_fd)));

  delegate_->StartTracing(trace_config, std::move(write_fd), callback);
}

void ArcTracingAgentImpl::StopAndFlush(tracing::mojom::RecorderPtr recorder) {
  recorder_ = std::move(recorder);
  StopAgentTracing(base::BindRepeating(&ArcTracingAgentImpl::RecorderProxy,
                                       base::Unretained(this)));
}

void ArcTracingAgentImpl::RecorderProxy(
    const std::string& event_name,
    const std::string& events_label,
    const scoped_refptr<base::RefCountedString>& events) {
  if (evens && !events->data().empty())
    recorder_->AddChunk(events->data());
  recorder_.reset();
}

void ArcTracingAgentImpl::RequestClockSyncMarker(
    const std::string& sync_id,
    const Agent::RequestClockSyncMarkerCallback& callback) {
  NOTREACHED();
}

void ArcTracingAgentImpl::GetCategories(
    const Agent::GetCategoriesCallback& callback) {
  callback.Run("");
}

void ArcTracingAgentImpl::RequestBufferStatus(
    const Agent::RequestBufferStatusCallback& callback) {
  callback.Run(0, 0);
}

ArcTracingAgent::Delegate::~Delegate() = default;

ArcTracingAgentImpl::ArcTracingReader::ArcTracingReader()
    : weak_ptr_factory_(this) {}

ArcTracingAgentImpl::ArcTracingReader::~ArcTracingReader() = default;

void ArcTracingAgentImpl::ArcTracingReader::StartTracing(
    base::ScopedFD read_fd) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  read_fd_ = std::move(read_fd);
  // We don't use the weak pointer returned by |GetWeakPtr| to avoid using it
  // on different task runner. Instead, we use |base::Unretained| here as
  // |fd_watcher_| is always destroyed before |this| is destroyed.
  fd_watcher_ = base::FileDescriptorWatcher::WatchReadable(
      read_fd_.get(), base::Bind(&ArcTracingReader::OnTraceDataAvailable,
                                 base::Unretained(this)));
}

void ArcTracingAgentImpl::ArcTracingReader::OnTraceDataAvailable() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  char buf[kArcTraceMessageLength];
  std::vector<base::ScopedFD> unused_fds;
  ssize_t n = base::UnixDomainSocket::RecvMsg(
      read_fd_.get(), buf, kArcTraceMessageLength, &unused_fds);
  // When EOF, return and do nothing. The clean up is done in StopTracing.
  if (n == 0)
    return;

  if (n < 0) {
    DPLOG(WARNING) << "Unexpected error while reading trace from client.";
    // Do nothing here as StopTracing will do the clean up and the existing
    // trace logs will be returned.
    return;
  }

  if (n > static_cast<ssize_t>(kArcTraceMessageLength)) {
    DLOG(WARNING) << "Unexpected data size when reading trace from client.";
    return;
  }
  ring_buffer_.SaveToBuffer(std::string(buf, n));
}

void ArcTracingAgentImpl::ArcTracingReader::StopTracing(
    const StopTracingCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  fd_watcher_.reset();
  read_fd_.reset();

  bool append_comma = false;
  std::string data;
  for (auto it = ring_buffer_.Begin(); it; ++it) {
    if (append_comma)
      data.append(",");
    else
      append_comma = true;
    data.append(**it);
  }
  ring_buffer_.Clear();

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(callback, base::RefCountedString::TakeString(&data)));
}

base::WeakPtr<ArcTracingAgentImpl::ArcTracingReader>
ArcTracingAgentImpl::ArcTracingReader::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace content

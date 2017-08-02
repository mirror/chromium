// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/queueing_connection_filter.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace content {

QueueingConnectionFilter::QueueingConnectionFilter(
    scoped_refptr<base::SequencedTaskRunner> io_task_runner,
    std::unique_ptr<service_manager::BinderRegistry> registry)
    : io_task_runner_(io_task_runner),
      registry_(std::move(registry)),
      weak_factory_(this) {
  // This will be reattached by any of the IO thread functions on first call.
  DETACH_FROM_THREAD(io_thread_checker_);
}

QueueingConnectionFilter::~QueueingConnectionFilter() {
  DCHECK_CALLED_ON_VALID_THREAD(io_thread_checker_);
}

base::Closure QueueingConnectionFilter::GetReleaseCallback() {
  return base::Bind(base::IgnoreResult(&base::TaskRunner::PostTask),
                    io_task_runner_, FROM_HERE,
                    base::Bind(&QueueingConnectionFilter::Release,
                               weak_factory_.GetWeakPtr()));
}

QueueingConnectionFilter::PendingRequest::PendingRequest(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe)
    : interface_name(interface_name),
      interface_pipe(std::move(interface_pipe)) {}

QueueingConnectionFilter::PendingRequest::~PendingRequest() = default;

void QueueingConnectionFilter::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe,
    service_manager::Connector* connector) {
  DCHECK_CALLED_ON_VALID_THREAD(io_thread_checker_);
  if (!registry_->CanBindInterface(interface_name))
    return;

  if (released_) {
    registry_->BindInterface(interface_name, std::move(*interface_pipe));
    return;
  }

  pending_requests_.emplace_back(base::MakeUnique<PendingRequest>(
      interface_name, std::move(*interface_pipe)));
}

void QueueingConnectionFilter::Release() {
  DCHECK_CALLED_ON_VALID_THREAD(io_thread_checker_);
  released_ = true;

  for (auto& request : pending_requests_) {
    registry_->BindInterface(request->interface_name,
                             std::move(request->interface_pipe));
  }
  pending_requests_.clear();
}

}  // namespace content

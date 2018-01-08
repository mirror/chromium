// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_interface_broker/binder_holder.h"

#include <utility>

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_context_type.h"
#include "content/public/browser/web_interface_filter.h"

namespace content {

BinderHolder::BinderHolder(
    std::string interface_name,
    std::unique_ptr<WebInterfaceFilter> filter,
    base::RepeatingCallback<void(const std::string&,
                                 mojo::ScopedMessagePipeHandle,
                                 RenderFrameHost*)> frame_binder,
    base::RepeatingCallback<void(const std::string&,
                                 mojo::ScopedInterfaceEndpointHandle,
                                 RenderFrameHost*)> associated_frame_binder,
    base::RepeatingCallback<void(const std::string&,
                                 mojo::ScopedMessagePipeHandle,
                                 RenderProcessHost*,
                                 const url::Origin&)> origin_binder)
    : interface_name_(std::move(interface_name)),
      filter_(std::move(filter)),
      frame_binder_(std::move(frame_binder)),
      associated_frame_binder_(std::move(associated_frame_binder)),
      origin_binder_(std::move(origin_binder)) {}

BinderHolder::BinderHolder(BinderHolder&&) = default;
BinderHolder& BinderHolder::operator=(BinderHolder&&) = default;
BinderHolder::~BinderHolder() = default;

void BinderHolder::BindInterfaceForWorker(
    WebContextType context_type,
    mojo::ScopedMessagePipeHandle interface_pipe,
    RenderProcessHost* host,
    const url::Origin& origin,
    mojo::ReportBadMessageCallback bad_message_callback) const {
  if (!origin_binder_) {
    ReportBadMessage(std::move(bad_message_callback));
    return;
  }
  if (!PassesFilter(context_type, nullptr, host, origin,
                    std::move(bad_message_callback))) {
    return;
  }
  origin_binder_.Run(interface_name_, std::move(interface_pipe), host, origin);
}

void BinderHolder::BindInterfaceForFrame(
    mojo::ScopedMessagePipeHandle interface_pipe,
    RenderFrameHost* frame,
    mojo::ReportBadMessageCallback bad_message_callback) const {
  if (!origin_binder_ && !frame_binder_) {
    ReportBadMessage(std::move(bad_message_callback));
    return;
  }
  if (!PassesFilter(WebContextType::kFrame, frame, frame->GetProcess(),
                    frame->GetLastCommittedOrigin(),
                    std::move(bad_message_callback))) {
    return;
  }
  if (frame_binder_) {
    frame_binder_.Run(interface_name_, std::move(interface_pipe), frame);
    return;
  }
  origin_binder_.Run(interface_name_, std::move(interface_pipe),
                     frame->GetProcess(), frame->GetLastCommittedOrigin());
}

void BinderHolder::BindAssociatedInterfaceForFrame(
    mojo::ScopedInterfaceEndpointHandle interface_pipe,
    RenderFrameHost* frame,
    mojo::ReportBadMessageCallback bad_message_callback) const {
  if (!associated_frame_binder_) {
    ReportBadMessage(std::move(bad_message_callback));
    return;
  }
  if (!PassesFilter(WebContextType::kFrame, frame, frame->GetProcess(),
                    frame->GetLastCommittedOrigin(),
                    std::move(bad_message_callback))) {
    return;
  }
  associated_frame_binder_.Run(interface_name_, std::move(interface_pipe),
                               frame);
}

bool BinderHolder::PassesFilter(
    WebContextType context_type,
    RenderFrameHost* frame,
    RenderProcessHost* process,
    const url::Origin& origin,
    mojo::ReportBadMessageCallback bad_message_callback) const {
  DCHECK(bad_message_callback);

  auto access_result =
      filter_->FilterInterface(context_type, process, frame, origin);
  switch (access_result) {
    case WebInterfaceFilter::Result::kBadMessage: {
      ReportBadMessage(std::move(bad_message_callback));
    }
    // Fall-through
    case WebInterfaceFilter::Result::kDeny: {
      DVLOG(1) << interface_name_ << " blocked";
      return false;
    }
    case WebInterfaceFilter::Result::kAllow: {
      return true;
    }
  }

  return false;
}

void BinderHolder::ReportBadMessage(
    mojo::ReportBadMessageCallback bad_message_callback) const {
  // Note: This currently does not kill the renderer process since the
  // InterfaceProvider connection is proxied via service manager. Once this is
  // complete and that proxying is removed, this will result in the renderer
  // making this request being killed.
  std::move(bad_message_callback)
      .Run("Bad request for interface " + interface_name_);
}

}  // namespace content

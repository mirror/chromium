// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/resource_scheduler_filter.h"

#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/browser/loader/resource_scheduler.h"
#include "content/common/frame_messages.h"
#include "ipc/ipc_message_macros.h"
#include "ui/base/page_transition_types.h"

namespace content {

ResourceSchedulerFilter::ResourceSchedulerFilter(int child_id)
    : BrowserMessageFilter(FrameMsgStart), child_id_(child_id) {}

ResourceSchedulerFilter::~ResourceSchedulerFilter() {}

bool ResourceSchedulerFilter::OnMessageReceived(const IPC::Message& message) {
  IPC_BEGIN_MESSAGE_MAP(ResourceSchedulerFilter, message)
    IPC_MESSAGE_HANDLER(FrameHostMsg_WillInsertBody, OnWillInsertBody)
  IPC_END_MESSAGE_MAP()
  return false;
}

// static
void ResourceSchedulerFilter::OnDidCommitMainframeNavigation(
    int render_process_id,
    int render_view_routing_id) {
  if (!ResourceDispatcherHostImpl::Get())
    return;
  ResourceScheduler* scheduler = ResourceDispatcherHostImpl::Get()->scheduler();
  if (!scheduler)
    return;
  scheduler->OnNavigate(render_process_id, render_view_routing_id);
}

void ResourceSchedulerFilter::OnWillInsertBody(int render_view_routing_id) {
  ResourceScheduler* scheduler = ResourceDispatcherHostImpl::Get()->scheduler();
  if (!scheduler)
    return;
  scheduler->OnWillInsertBody(child_id_, render_view_routing_id);
}

}  // namespace content

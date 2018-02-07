// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/render_frame_metadata_observer_impl.h"

#include "content/common/view_messages.h"
#include "content/renderer/render_thread_impl.h"

namespace content {

RenderFrameMetadataObserverImpl::RenderFrameMetadataObserverImpl(
    int32_t routing_id)
    : routing_id_(routing_id),
      message_filter_(new RenderWidgetMessageFilter(this)),
      sync_message_filter_(RenderThreadImpl::current()->sync_message_filter()) {
  RenderThread::Get()->AddFilter(message_filter_.get());
}

RenderFrameMetadataObserverImpl::~RenderFrameMetadataObserverImpl() {
  // We are created on the RenderThread, but owned by the compositor thread.
  // Verify that the RenderThread is alive so that we can post task to it.
  if (RenderThread::Get())
    RenderThread::Get()->RemoveFilter(message_filter_.get());
}

void RenderFrameMetadataObserverImpl::SetEnabled(bool enabled,
                                                 int requested_routing_id) {
  // If there is no sync_message_filter then this is being used by a test where
  // there is no gpu process, and therefore no frame submission. Crash so that
  // the test can then be fixed.
  CHECK(sync_message_filter_);
  RenderFrameMetadataObserver::SetEnabled(enabled);
  requested_routing_id_ = requested_routing_id;
}

void RenderFrameMetadataObserverImpl::OnRenderFrameSubmission(
    cc::RenderFrameMetadata* metadata) {
  // Notify the widget
  sync_message_filter_->Send(
      new ViewHostMsg_OnRenderFrameSubmitted(routing_id_, *metadata));

  // Notify the test interface.
  if (requested_routing_id_) {
    sync_message_filter_->Send(new ViewHostMsg_OnRenderFrameSubmitted(
        requested_routing_id_, *metadata));
  }
}

void RenderFrameMetadataObserverImpl::SetCompositorTaskRunner(
    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner) {
  message_filter_->SetCompositorTaskRunner(compositor_task_runner);
}

RenderFrameMetadataObserverImpl::RenderWidgetMessageFilter::
    RenderWidgetMessageFilter(RenderFrameMetadataObserverImpl* impl)
    : impl_(impl) {}

RenderFrameMetadataObserverImpl::RenderWidgetMessageFilter::
    ~RenderWidgetMessageFilter() {}

void RenderFrameMetadataObserverImpl::RenderWidgetMessageFilter::
    SetCompositorTaskRunner(
        scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner) {
  compositor_task_runner_ = compositor_task_runner;
}

bool RenderFrameMetadataObserverImpl::RenderWidgetMessageFilter::
    OnMessageReceived(const IPC::Message& message) {
  // TODO(jonross): add generic control signals for non test observers in the
  // browser process. Consume those.
  IPC_BEGIN_MESSAGE_MAP(RenderWidgetMessageFilter, message)
    IPC_MESSAGE_HANDLER(ViewMsg_WaitForNextFrameForTests,
                        OnWaitNextFrameForTests)
  IPC_END_MESSAGE_MAP()
  // Currently do not block the propogation of this event.
  // TODO(jonross): eliminate all usage in RenderWidget
  return false;
}

void RenderFrameMetadataObserverImpl::RenderWidgetMessageFilter::
    OnWaitNextFrameForTests(int routing_id) {
  if (compositor_task_runner_) {
    compositor_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&RenderFrameMetadataObserverImpl::SetEnabled,
                                  base::Unretained(impl_), true, routing_id));
  }
}

}  // namespace content

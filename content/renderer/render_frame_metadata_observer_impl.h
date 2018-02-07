// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_RENDER_FRAME_METADATA_OBSERVER_IMPL_H_
#define CONTENT_RENDERER_RENDER_FRAME_METADATA_OBSERVER_IMPL_H_

#include "cc/trees/render_frame_metadata_observer.h"
#include "ipc/message_filter.h"

namespace cc {
class RenderFrameMetadata;
}  // namespace cc

namespace IPC {
class SyncMessageFilter;
}  // namespace IPC

namespace content {

// Implementation of cc::RenderFrameMetadataObserver which observers frame
// submission, then notifies both the widget, and optional test interface, of
// the metadata.
//
// This class operates across three threads.
//   - Creation occurs on the renderer's main thread.
//   - IPCs to/from the browser process occur on the IO thread.
//   - Notifications of frame submission occur on the compositor thread.
//
// This class instantiates an IPC::MessageFilter which it uses to receive
// command signals from the browser process on the IO thread. This is used so
// that the messages are received before targeting, which occurs on the main
// thread, which can be blocked.
class RenderFrameMetadataObserverImpl : public cc::RenderFrameMetadataObserver {
 public:
  RenderFrameMetadataObserverImpl(int32_t routing_id);
  ~RenderFrameMetadataObserverImpl() override;

  // Should be ran on the compositor thread. Updates our enabled state to begin
  // observing frame submission. Also sets |requested_routing_id| for tests.
  void SetEnabled(bool enabled, int requested_routing_id);

  // cc::RenderFrameMetadataObserver:
  void OnRenderFrameSubmission(cc::RenderFrameMetadata* metadata) override;
  void SetCompositorTaskRunner(scoped_refptr<base::SingleThreadTaskRunner>
                                   compositor_task_runner) override;

 private:
  // Receives control signals from the browser proces on the IO thread. Then
  // posts handling of these messages to the compositor thread so that
  // RenderFrameMetadataObserverImpl can be updated appropriately
  class RenderWidgetMessageFilter : public IPC::MessageFilter {
   public:
    RenderWidgetMessageFilter(RenderFrameMetadataObserverImpl* impl);

    // Sets the task runner so that callbacks to RenderFrameMetadataObserverImpl
    // can be ran on the compositor thread.
    void SetCompositorTaskRunner(
        scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner);

    // IPC::MessageFilter:
    bool OnMessageReceived(const IPC::Message& message) override;

   protected:
    ~RenderWidgetMessageFilter() override;

   private:
    // Handles the ipc for ViewMsg_WaitForNextFrameForTests
    void OnWaitNextFrameForTests(int routing_id);

    // Task runner for compositor thread. Used to run callbacks into
    // RenderFrameMetadataObserverImpl.
    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_ =
        nullptr;
    // Owns |this| used to update enabled state based on ipcs.
    RenderFrameMetadataObserverImpl* impl_;
    DISALLOW_COPY_AND_ASSIGN(RenderWidgetMessageFilter);
  };

  // The routing id requested by a testing interface.
  int32_t requested_routing_id_ = 0;

  // The routing id for the widget.
  const int32_t routing_id_;

  // IPC::MessageFilter to receive control signals on the IO thread.
  scoped_refptr<RenderWidgetMessageFilter> message_filter_;
  // IPC::Sender used to send ipcs to the browser process on the IO thread.
  scoped_refptr<IPC::SyncMessageFilter> sync_message_filter_;

  DISALLOW_COPY_AND_ASSIGN(RenderFrameMetadataObserverImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_RENDER_FRAME_METADATA_OBSERVER_IMPL_H_

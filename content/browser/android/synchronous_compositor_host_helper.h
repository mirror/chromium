// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_SYNCHRONOUS_COMPOSITOR_HOST_HELPER_H_
#define CONTENT_BROWSER_ANDROID_SYNCHRONOUS_COMPOSITOR_HOST_HELPER_H_

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "content/common/input/sync_compositor_messages.h"
#include "content/public/browser/android/synchronous_compositor.h"

namespace content {

class SynchronousCompositorBrowserFilter;
class SynchronousCompositorHost;

// This class is accessed both on the IO thread and the main thread.
class SynchronousCompositorHostHelper
    : public base::RefCountedThreadSafe<SynchronousCompositorHostHelper> {
 public:
  SynchronousCompositorHostHelper(SynchronousCompositorHost* host);

  // Attach a filter.
  void BindFilter(scoped_refptr<SynchronousCompositorBrowserFilter> filter);

  // Indicatation that the remote is now ready to process requests. Called
  // on IO thread.
  void RemoteReady();

  // Remote channel is closed signal all waiters. Called on IO thread.
  void RemoteClosed();

  // Receive a frame. Return false if the corresponding frame wasn't found.
  // Called on IO thread.
  bool ReceiveFrame(int frame_sink_id, base::Optional<viz::CompositorFrame>&);

  // Receive a BeginFrameResponse. Called on IO thread.
  void BeginFrameResponse(
      const SyncCompositorCommonRendererParams& render_params);

  // Schedule a callback for when vsync finishes and wait for the
  // BeginFrameResponse callback. Called on UI thread.
  void WaitAfterVSync(ui::WindowAndroid* window_android);

  // Store a FrameFuture for a later ReceiveFrame callback. Called on UI thread.
  void SetFrameFuture(
      scoped_refptr<SynchronousCompositor::FrameFuture> frame_future);

  // Indicate the host is destroyed. Called on UI thread.
  void HostDestroyed();

 private:
  friend class base::RefCountedThreadSafe<SynchronousCompositorHostHelper>;
  ~SynchronousCompositorHostHelper();

  // Callback passed to WindowAndroid, runs when the current vsync is completed.
  // Called on UI thread.
  void VSyncComplete();

  // Process metadata. Called on UI thread.
  void ProcessFrameMetadata(viz::CompositorFrameMetadata metadata);

  using FrameFutureQueue =
      base::circular_deque<scoped_refptr<SynchronousCompositor::FrameFuture>>;

  const int routing_id_;

  // UI thread only.
  ui::WindowAndroid* window_android_in_vsync_ = nullptr;
  SynchronousCompositorHost* host_;

  // Shared variables between the IO thread and UI thread.
  base::Lock lock_;
  FrameFutureQueue frame_futures_;
  bool begin_frame_response_valid_ = false;
  SyncCompositorCommonRendererParams last_render_params_;
  base::ConditionVariable begin_frame_condition_;
  bool remote_closed_ = true;

  scoped_refptr<SynchronousCompositorBrowserFilter> filter_;

  DISALLOW_COPY_AND_ASSIGN(SynchronousCompositorHostHelper);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_SYNCHRONOUS_COMPOSITOR_HOST_HELPER_H_

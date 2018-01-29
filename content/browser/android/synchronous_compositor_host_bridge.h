// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_SYNCHRONOUS_COMPOSITOR_HOST_BRIDGE_H_
#define CONTENT_BROWSER_ANDROID_SYNCHRONOUS_COMPOSITOR_HOST_BRIDGE_H_

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
class SynchronousCompositorHostBridge
    : public base::RefCountedThreadSafe<SynchronousCompositorHostBridge> {
 public:
  explicit SynchronousCompositorHostBridge(SynchronousCompositorHost* host);

  // Attach a filter.
  void BindFilterOnUIThread();

  // Indicatation that the remote is now ready to process requests. Called
  // on either thread.
  void RemoteReady();

  // Remote channel is closed signal all waiters.
  void RemoteClosedOnIOThread();

  // Receive a frame. Return false if the corresponding frame wasn't found.
  bool ReceiveFrameOnIOThread(int frame_sink_id,
                              base::Optional<viz::CompositorFrame>&);

  // Receive a BeginFrameResponse. Return if handling the response was
  // successful or not.
  bool BeginFrameResponseOnIOThread(
      const SyncCompositorCommonRendererParams& render_params);

  // Schedule a callback for when vsync finishes and wait for the
  // BeginFrameResponse callback.
  bool WaitAfterVSyncOnUIThread(ui::WindowAndroid* window_android);

  // Store a FrameFuture for a later ReceiveFrame callback.
  void SetFrameFutureOnUIThread(
      scoped_refptr<SynchronousCompositor::FrameFuture> frame_future);

  // Indicate the host is destroyed.
  void HostDestroyedOnUIThread();

 private:
  friend class base::RefCountedThreadSafe<SynchronousCompositorHostBridge>;
  ~SynchronousCompositorHostBridge();

  // Callback passed to WindowAndroid, runs when the current vsync is completed.
  void VSyncCompleteOnUIThread();

  // Process metadata.
  void ProcessFrameMetadataOnUIThread(viz::CompositorFrameMetadata metadata);

  void UnregisterHostOnIOThread(
      scoped_refptr<SynchronousCompositorBrowserFilter> filter);

  using FrameFutureQueue =
      base::circular_deque<scoped_refptr<SynchronousCompositor::FrameFuture>>;

  const int routing_id_;

  // UI thread only.
  ui::WindowAndroid* window_android_in_vsync_ = nullptr;
  SynchronousCompositorHost* host_;
  bool bound_to_filter_ = false;

  // Shared variables between the IO thread and UI thread.
  base::Lock lock_;
  FrameFutureQueue frame_futures_;
  bool begin_frame_response_valid_ = false;
  SyncCompositorCommonRendererParams last_render_params_;
  base::ConditionVariable begin_frame_condition_;
  bool remote_closed_ = true;

  // IO thread based callback that will unbind this object from
  // the SynchronousCompositorBrowserFilter. Only called once
  // all pending frames are acknowledged.
  base::OnceClosure unregister_callback_;

  DISALLOW_COPY_AND_ASSIGN(SynchronousCompositorHostBridge);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_SYNCHRONOUS_COMPOSITOR_HOST_BRIDGE_H_

// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/synchronous_compositor_sync_call_bridge.h"

#include "content/browser/android/synchronous_compositor_browser_filter.h"
#include "content/browser/android/synchronous_compositor_host.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/public/browser/browser_thread.h"
#include "ui/android/window_android.h"

namespace content {

SynchronousCompositorSyncCallBridge::SynchronousCompositorSyncCallBridge(
    SynchronousCompositorHost* host)
    : routing_id_(host->routing_id()),
      host_(host),
      begin_frame_condition_(&lock_) {
  DCHECK(host);
}

SynchronousCompositorSyncCallBridge::~SynchronousCompositorSyncCallBridge() {
  DCHECK(frame_futures_.empty());
  DCHECK(!window_android_in_vsync_);
}

void SynchronousCompositorSyncCallBridge::BindFilterOnUIThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(host_);
  if (bound_to_filter_)
    return;
  scoped_refptr<SynchronousCompositorBrowserFilter> filter = host_->GetFilter();
  if (!filter)
    return;
  bound_to_filter_ = true;
  scoped_refptr<SynchronousCompositorSyncCallBridge> thiz = this;
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &SynchronousCompositorBrowserFilter::RegisterSyncCallBridge,
          std::move(filter), routing_id_, thiz));
}

void SynchronousCompositorSyncCallBridge::RemoteReady() {
  base::AutoLock lock(lock_);
  remote_closed_ = false;
}

void SynchronousCompositorSyncCallBridge::RemoteClosedOnIOThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  base::AutoLock lock(lock_);
  SignalRemoteClosedToAllWaitersOnIOThread();
  UnregisterSyncCallBridgeIfNecessary();
}

bool SynchronousCompositorSyncCallBridge::ReceiveFrameOnIOThread(
    int layer_tree_frame_sink_id,
    base::Optional<viz::CompositorFrame> compositor_frame) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  base::AutoLock lock(lock_);
  if (remote_closed_ || frame_futures_.empty())
    return false;
  auto frame_ptr = std::make_unique<SynchronousCompositor::Frame>();
  frame_ptr->layer_tree_frame_sink_id = layer_tree_frame_sink_id;
  scoped_refptr<SynchronousCompositor::FrameFuture> future =
      std::move(frame_futures_.front());
  DCHECK(future);
  frame_futures_.pop_front();

  if (compositor_frame) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&SynchronousCompositorSyncCallBridge::
                           ProcessFrameMetadataOnUIThread,
                       this, compositor_frame->metadata.Clone()));
    frame_ptr->frame.reset(new viz::CompositorFrame);
    *frame_ptr->frame = std::move(*compositor_frame);
  }
  future->SetFrame(std::move(frame_ptr));

  UnregisterSyncCallBridgeIfNecessary();
  return true;
}

bool SynchronousCompositorSyncCallBridge::BeginFrameResponseOnIOThread(
    const SyncCompositorCommonRendererParams& render_params) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  base::AutoLock lock(lock_);
  if (begin_frame_response_valid_)
    return false;
  begin_frame_response_valid_ = true;
  last_render_params_ = render_params;
  begin_frame_condition_.Signal();
  return true;
}

bool SynchronousCompositorSyncCallBridge::WaitAfterVSyncOnUIThread(
    ui::WindowAndroid* window_android) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  base::AutoLock lock(lock_);
  if (remote_closed_)
    return false;
  DCHECK(!begin_frame_response_valid_);
  DCHECK(bound_to_filter_);
  if (window_android_in_vsync_) {
    DCHECK_EQ(window_android_in_vsync_, window_android);
    return true;
  }
  window_android_in_vsync_ = window_android;
  window_android_in_vsync_->AddVSyncCompleteCallback(base::BindRepeating(
      &SynchronousCompositorSyncCallBridge::VSyncCompleteOnUIThread, this));
  return true;
}

void SynchronousCompositorSyncCallBridge::SetFrameFutureOnUIThread(
    scoped_refptr<SynchronousCompositor::FrameFuture> frame_future) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(frame_future);
  base::AutoLock lock(lock_);
  if (remote_closed_) {
    frame_future->SetFrame(nullptr);
    return;
  }

  BindFilterOnUIThread();
  DCHECK(bound_to_filter_);
  // Allowing arbitrary number of pending futures can lead to increase in frame
  // latency. Due to this, Android platform already ensures that here that there
  // can be at most 2 pending frames. Here, we rely on Android to do the
  // necessary blocking, which allows more parallelism without increasing
  // latency. But DCHECK Android blocking is working.
  DCHECK_LT(frame_futures_.size(), 2u);
  frame_futures_.emplace_back(std::move(frame_future));
}

void SynchronousCompositorSyncCallBridge::HostDestroyedOnUIThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(host_);
  scoped_refptr<SynchronousCompositorBrowserFilter> filter = host_->GetFilter();

  base::AutoLock lock(lock_);
  bound_to_filter_ = false;
  host_ = nullptr;

  if (filter) {
    unregister_callback_ =
        base::BindOnce(&SynchronousCompositorSyncCallBridge::
                           UnregisterSyncCallBridgeOnIOThread,
                       this, filter);

    UnregisterSyncCallBridgeIfNecessary();
  }
}

void SynchronousCompositorSyncCallBridge::VSyncCompleteOnUIThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(window_android_in_vsync_);
  window_android_in_vsync_ = nullptr;

  base::AutoLock lock(lock_);
  if (remote_closed_)
    return;

  // If we haven't received a response yet. Wait for it.
  if (!begin_frame_response_valid_) {
    base::ScopedAllowBaseSyncPrimitivesOutsideBlockingScope
        allow_base_sync_primitives;
    begin_frame_condition_.Wait();
  }
  DCHECK(begin_frame_response_valid_ || remote_closed_);
  begin_frame_response_valid_ = false;
  if (!remote_closed_)
    host_->ProcessCommonParams(last_render_params_);
}

void SynchronousCompositorSyncCallBridge::ProcessFrameMetadataOnUIThread(
    viz::CompositorFrameMetadata metadata) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (host_)
    host_->UpdateFrameMetaData(std::move(metadata));
}

void SynchronousCompositorSyncCallBridge::UnregisterSyncCallBridgeOnIOThread(
    scoped_refptr<SynchronousCompositorBrowserFilter> filter) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  {
    base::AutoLock lock(lock_);
    if (!remote_closed_)
      SignalRemoteClosedToAllWaitersOnIOThread();
  }
  if (filter)
    filter->UnregisterSyncCallBridge(routing_id_);
}

void SynchronousCompositorSyncCallBridge::
    SignalRemoteClosedToAllWaitersOnIOThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  lock_.AssertAcquired();
  remote_closed_ = true;
  for (auto& future_ptr : frame_futures_) {
    future_ptr->SetFrame(nullptr);
  }
  frame_futures_.clear();
  begin_frame_condition_.Signal();
}

void SynchronousCompositorSyncCallBridge::
    UnregisterSyncCallBridgeIfNecessary() {
  // Can be called from either thread.
  lock_.AssertAcquired();

  // If no more frames left deregister if necessary. Post a task as
  // unregistering will destroy this object.
  if (frame_futures_.empty() && unregister_callback_) {
    BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                            std::move(unregister_callback_));
  }
}

}  // namespace content

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/synchronous_compositor_browser_filter.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/lazy_instance.h"
#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "base/stl_util.h"
#include "content/browser/android/synchronous_compositor_host.h"
#include "content/browser/bad_message.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_features.h"
#include "ui/android/window_android.h"

namespace content {

SynchronousCompositorBrowserFilter::SynchronousCompositorBrowserFilter(
    int process_id)
    : BrowserMessageFilter(SyncCompositorMsgStart),
      render_process_host_(RenderProcessHost::FromID(process_id)),
      use_mojo_(base::FeatureList::IsEnabled(features::kMojoInputMessages)),
      sync_condition_(&sync_lock_) {
  DCHECK(render_process_host_);
}

SynchronousCompositorBrowserFilter::~SynchronousCompositorBrowserFilter() {
  DCHECK(pending_renderer_state_.empty());
  DCHECK(future_map_.empty());
}

void SynchronousCompositorBrowserFilter::SendBeginFrameAndSyncAfterVSync(
    ui::WindowAndroid* window_android,
    SynchronousCompositorHost* compositor_host,
    const viz::BeginFrameArgs& args) {
  DCHECK(!window_android_in_vsync_ ||
         window_android_in_vsync_ == window_android)
      << !!window_android_in_vsync_;
  DCHECK(compositor_host);
  DCHECK(pending_renderer_state_.end() ==
         pending_renderer_state_.find(compositor_host->routing_id()));

  if (use_mojo_) {
    base::AutoLock lock(sync_lock_);
    pending_renderer_state_[compositor_host->routing_id()] = compositor_host;
  }
  bool res = compositor_host->SendBeginFrame(args);

  // check if the begin frame call was successful or not; if not
  // decrement the count.
  if (use_mojo_ && !res) {
    base::AutoLock lock(sync_lock_);
    pending_renderer_state_.erase(compositor_host->routing_id());
    return;
  }

  if (window_android_in_vsync_)
    return;
  window_android_in_vsync_ = window_android;
  window_android_in_vsync_->AddVSyncCompleteCallback(
      base::Bind(&SynchronousCompositorBrowserFilter::VSyncComplete, this));
}

bool SynchronousCompositorBrowserFilter::OnMessageReceived(
    const IPC::Message& message) {
  if (use_mojo_)
    return false;

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(SynchronousCompositorBrowserFilter, message)
    IPC_MESSAGE_HANDLER_GENERIC(SyncCompositorHostMsg_ReturnFrame,
                                ReceiveFrame(message))
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

bool SynchronousCompositorBrowserFilter::ReceiveFrame(
    const IPC::Message& message) {
  SyncCompositorHostMsg_ReturnFrame::Param param;
  if (!SyncCompositorHostMsg_ReturnFrame::Read(&message, &param))
    return false;

  int routing_id = message.routing_id();
  uint32_t layer_tree_frame_sink_id = std::get<0>(param);
  base::Optional<viz::CompositorFrame>& compositor_frame = std::get<1>(param);
  ReturnFrame(routing_id, layer_tree_frame_sink_id,
              std::move(compositor_frame));
  return true;
}

void SynchronousCompositorBrowserFilter::ProcessFrameMetadataOnUIThread(
    int routing_id,
    viz::CompositorFrameMetadata metadata) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto itr = hosts_.find(routing_id);
  if (itr == hosts_.end())
    return;
  itr->second->UpdateFrameMetaData(std::move(metadata));
}

void SynchronousCompositorBrowserFilter::RegisterHost(
    SynchronousCompositorHost* host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(host);
  DCHECK(!base::ContainsKey(hosts_, host->routing_id()));
  hosts_[host->routing_id()] = host;
}

void SynchronousCompositorBrowserFilter::UnregisterHost(
    SynchronousCompositorHost* host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(host);
  int routing_id = host->routing_id();
  DCHECK(base::ContainsKey(hosts_, routing_id));
  DCHECK_EQ(host, hosts_[routing_id]);
  hosts_.erase(routing_id);
}

void SynchronousCompositorBrowserFilter::SetFrameFuture(
    int routing_id,
    scoped_refptr<SynchronousCompositor::FrameFuture> frame_future) {
  DCHECK(frame_future);
  base::AutoLock lock(future_map_lock_);
  if (!wait_on_futures_) {
    frame_future->SetFrame(nullptr);
    return;
  }

  auto itr = future_map_.find(routing_id);
  if (itr == future_map_.end()) {
    auto emplace_result = future_map_.emplace(routing_id, FrameFutureQueue());
    DCHECK(emplace_result.second);
    itr = emplace_result.first;
  }

  // Allowing arbitrary number of pending futures can lead to increase in frame
  // latency. Due to this, Android platform already ensures that here that there
  // can be at most 2 pending frames. Here, we rely on Android to do the
  // necessary blocking, which allows more parallelism without increasing
  // latency. But DCHECK Android blocking is working.
  DCHECK_LT(itr->second.size(), 2u);
  itr->second.emplace_back(std::move(frame_future));
}

void SynchronousCompositorBrowserFilter::ReturnFrame(
    int routing_id,
    uint32_t layer_tree_frame_sink_id,
    base::Optional<viz::CompositorFrame> compositor_frame) {
  scoped_refptr<SynchronousCompositor::FrameFuture> future;
  {
    base::AutoLock lock(future_map_lock_);
    auto itr = future_map_.find(routing_id);
    if (itr == future_map_.end() || itr->second.empty()) {
      // Be careful not to terminate the process if the channel is closing and
      // we've signal all futures but received a notification afterwards.
      if (wait_on_futures_) {
        bad_message::ReceivedBadMessage(this,
                                        bad_message::SCO_INVALID_ARGUMENT);
      }
      return;
    }
    future = std::move(itr->second.front());
    DCHECK(future);
    itr->second.pop_front();
    if (itr->second.empty())
      future_map_.erase(itr);
  }

  auto frame_ptr = std::make_unique<SynchronousCompositor::Frame>();
  frame_ptr->layer_tree_frame_sink_id = layer_tree_frame_sink_id;
  if (compositor_frame) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(
            &SynchronousCompositorBrowserFilter::ProcessFrameMetadataOnUIThread,
            this, routing_id,
            base::Passed(compositor_frame->metadata.Clone())));
    frame_ptr->frame.reset(new viz::CompositorFrame);
    *frame_ptr->frame = std::move(*compositor_frame);
  }
  future->SetFrame(std::move(frame_ptr));
}

void SynchronousCompositorBrowserFilter::BeginFrameResponse(
    int routing_id,
    const content::SyncCompositorCommonRendererParams& params) {
  base::AutoLock lock(sync_lock_);
  if (pending_renderer_state_.find(routing_id) ==
          pending_renderer_state_.end() ||
      pending_sync_params_.find(routing_id) != pending_sync_params_.end()) {
    bad_message::ReceivedBadMessage(this, bad_message::SCO_INVALID_ARGUMENT);
    return;
  }

  pending_sync_params_[routing_id] = params;
  sync_condition_.Signal();
}

void SynchronousCompositorBrowserFilter::RemoteClosed(int routing_id) {
  // If there are any pending renderer states put a fake one in.
  {
    base::AutoLock lock(sync_lock_);
    auto itr = pending_renderer_state_.find(routing_id);
    if (itr != pending_renderer_state_.end() &&
        pending_sync_params_.find(routing_id) == pending_sync_params_.end()) {
      pending_sync_params_[routing_id] =
          content::SyncCompositorCommonRendererParams();
      sync_condition_.Signal();
    }
  }

  // If there are any futures waiting for the routing id
  // signal them.
  {
    base::AutoLock lock(future_map_lock_);
    auto itr = future_map_.find(routing_id);
    if (itr != future_map_.end()) {
      for (auto& future_ptr : itr->second) {
        future_ptr->SetFrame(nullptr);
      }
    }
  }
}

void SynchronousCompositorBrowserFilter::OnFilterAdded(IPC::Channel* channel) {
  base::AutoLock lock(future_map_lock_);
  wait_on_futures_ = true;
}

void SynchronousCompositorBrowserFilter::OnFilterRemoved() {
  SignalAllFutures();
}

void SynchronousCompositorBrowserFilter::OnChannelClosing() {
  SignalAllFutures();
}

void SynchronousCompositorBrowserFilter::SignalAllFutures() {
  base::AutoLock lock(future_map_lock_);
  for (auto& pair : future_map_) {
    for (auto& future_ptr : pair.second) {
      future_ptr->SetFrame(nullptr);
    }
  }
  future_map_.clear();
  wait_on_futures_ = false;
}

void SynchronousCompositorBrowserFilter::VSyncComplete() {
  DCHECK(window_android_in_vsync_);
  window_android_in_vsync_ = nullptr;

  if (use_mojo_) {
    base::AutoLock lock(sync_lock_);
    {
      base::ScopedAllowBaseSyncPrimitivesOutsideBlockingScope
          allow_base_sync_primitives;
      while (pending_renderer_state_.size() != pending_sync_params_.size()) {
        sync_condition_.Wait();
      }
    }

    for (auto& pending_state : pending_renderer_state_) {
      auto sync_itr = pending_sync_params_.find(pending_state.first);
      if (sync_itr == pending_sync_params_.end()) {
        bad_message::ReceivedBadMessage(this,
                                        bad_message::SCO_INVALID_ARGUMENT);
        continue;
      }
      pending_state.second->UpdateState(sync_itr->second);
    }
    pending_sync_params_.clear();
  } else {
    std::vector<SyncCompositorCommonRendererParams> params;
    params.reserve(pending_renderer_state_.size());

    std::vector<int> routing_ids;
    for (auto& pending_state : pending_renderer_state_)
      routing_ids.push_back(pending_state.first);

    {
      base::ScopedAllowBaseSyncPrimitivesOutsideBlockingScope
          allow_base_sync_primitives;
      if (!render_process_host_->Send(
              new SyncCompositorMsg_SynchronizeRendererState(routing_ids,
                                                             &params))) {
        pending_renderer_state_.clear();
        return;
      }
    }

    if (pending_renderer_state_.size() != params.size()) {
      bad_message::ReceivedBadMessage(render_process_host_,
                                      bad_message::SCO_INVALID_ARGUMENT);
      pending_renderer_state_.clear();
      return;
    }
    size_t i = 0;
    for (auto& pending_state : pending_renderer_state_) {
      DCHECK_EQ(pending_state.first, routing_ids[i]);
      pending_state.second->UpdateState(params[i]);
      ++i;
    }
  }

  pending_renderer_state_.clear();
}

}  // namespace content

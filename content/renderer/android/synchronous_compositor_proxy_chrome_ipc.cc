// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/android/synchronous_compositor_proxy_chrome_ipc.h"

#include "base/auto_reset.h"
#include "base/command_line.h"
#include "base/memory/shared_memory.h"
#include "cc/ipc/cc_param_traits.h"
#include "content/common/android/sync_compositor_statics.h"
#include "content/common/input/sync_compositor_messages.h"
#include "content/public/common/content_switches.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_sender.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/gfx/skia_util.h"

namespace content {

SynchronousCompositorProxyChromeIPC::SynchronousCompositorProxyChromeIPC(
    int routing_id,
    IPC::Sender* sender,
    ui::SynchronousInputHandlerProxy* input_handler_proxy)
    : SynchronousCompositorProxy(input_handler_proxy),
      routing_id_(routing_id),
      sender_(sender) {}

SynchronousCompositorProxyChromeIPC::~SynchronousCompositorProxyChromeIPC() {}

void SynchronousCompositorProxyChromeIPC::Send(IPC::Message* message) {
  sender_->Send(message);
}

void SynchronousCompositorProxyChromeIPC::OnMessageReceived(
    const IPC::Message& message) {
  LOG(ERROR) << "Sync compositor message";
  IPC_BEGIN_MESSAGE_MAP(SynchronousCompositorProxyChromeIPC, message)
    IPC_MESSAGE_HANDLER(SyncCompositorMsg_ComputeScroll, ComputeScroll)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(SyncCompositorMsg_DemandDrawHw,
                                    OnDemandDrawHw)
    IPC_MESSAGE_HANDLER(SyncCompositorMsg_DemandDrawHwAsync,
                        OnDemandDrawHwAsync)
    IPC_MESSAGE_HANDLER(SyncCompositorMsg_SetSharedMemory, OnSetSharedMemory)
    IPC_MESSAGE_HANDLER(SyncCompositorMsg_ZeroSharedMemory, ZeroSharedMemory)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(SyncCompositorMsg_DemandDrawSw,
                                    OnDemandDrawSw)
    IPC_MESSAGE_HANDLER(SyncCompositorMsg_ZoomBy, OnZoomBy)
    IPC_MESSAGE_HANDLER(SyncCompositorMsg_SetScroll, OnSetScroll)
    IPC_MESSAGE_HANDLER(SyncCompositorMsg_SetMemoryPolicy, SetMemoryPolicy)
    IPC_MESSAGE_HANDLER(SyncCompositorMsg_ReclaimResources, ReclaimResources)
  IPC_END_MESSAGE_MAP()
}

void SynchronousCompositorProxyChromeIPC::OnDemandDrawHwAsync(
    const SyncCompositorDemandDrawHwParams& params) {
  DemandDrawHw(
      params,
      base::BindOnce(
          &SynchronousCompositorProxyChromeIPC::SendDemandDrawHwReplyAsync,
          base::Unretained(this)));
}

void SynchronousCompositorProxyChromeIPC::OnDemandDrawHw(
    const SyncCompositorDemandDrawHwParams& params,
    IPC::Message* reply_message) {
  DCHECK(reply_message);
  DemandDrawHw(params,
               base::BindOnce(
                   &SynchronousCompositorProxyChromeIPC::SendDemandDrawHwReply,
                   base::Unretained(this), reply_message));
}

void SynchronousCompositorProxyChromeIPC::OnDemandDrawSw(
    const SyncCompositorDemandDrawSwParams& params,
    IPC::Message* reply_message) {
  DemandDrawSw(params,
               base::BindOnce(
                   &SynchronousCompositorProxyChromeIPC::SendDemandDrawSwReply,
                   base::Unretained(this), reply_message));

  if (software_draw_reply_) {
    // Did not swap.
    SyncCompositorCommonRendererParams common_renderer_params;
    PopulateCommonParams(&common_renderer_params);
    std::move(software_draw_reply_).Run(common_renderer_params, base::nullopt);
  }
}

void SynchronousCompositorProxyChromeIPC::SendDemandDrawHwReplyAsync(
    const SyncCompositorCommonRendererParams& common_renderer_params,
    uint32_t layer_tree_frame_sink_id,
    base::Optional<viz::CompositorFrame> frame) {
  Send(new SyncCompositorHostMsg_ReturnFrame(routing_id_,
                                             layer_tree_frame_sink_id, frame));
}

void SynchronousCompositorProxyChromeIPC::SendDemandDrawHwReply(
    IPC::Message* reply_message,
    const SyncCompositorCommonRendererParams& common_renderer_params,
    uint32_t layer_tree_frame_sink_id,
    base::Optional<viz::CompositorFrame> frame) {
  SyncCompositorMsg_DemandDrawHw::WriteReplyParams(
      reply_message, common_renderer_params, layer_tree_frame_sink_id, frame);
  Send(reply_message);
}

void SynchronousCompositorProxyChromeIPC::SendDemandDrawSwReply(
    IPC::Message* reply_message,
    const SyncCompositorCommonRendererParams& common_renderer_params,
    base::Optional<viz::CompositorFrameMetadata> metadata) {
  SyncCompositorMsg_DemandDrawSw::WriteReplyParams(
      reply_message, common_renderer_params, metadata);
  Send(reply_message);
}

void SynchronousCompositorProxyChromeIPC::SendAsyncRendererStateIfNeeded() {
  if (hardware_draw_reply_ || software_draw_reply_ || zoom_by_reply_)
    return;
  SyncCompositorCommonRendererParams params;
  PopulateCommonParams(&params);

  Send(new SyncCompositorHostMsg_UpdateState(routing_id_, params));
}

void SynchronousCompositorProxyChromeIPC::OnSetSharedMemory(
    const SyncCompositorSetSharedMemoryParams& params,
    bool* success,
    SyncCompositorCommonRendererParams* common_renderer_params) {
  *success = false;
  SetSharedMemory(
      params,
      base::BindOnce(&SynchronousCompositorProxyChromeIPC::SetSharedMemoryReply,
                     base::Unretained(this), success, common_renderer_params));
}

void SynchronousCompositorProxyChromeIPC::SetSharedMemoryReply(
    bool* out_success,
    SyncCompositorCommonRendererParams* out_common_renderer_params,
    bool success,
    const SyncCompositorCommonRendererParams& common_renderer_params) {
  *out_success = success;
  *out_common_renderer_params = common_renderer_params;
}

void SynchronousCompositorProxyChromeIPC::OnZoomBy(
    float zoom_delta,
    const gfx::Point& anchor,
    SyncCompositorCommonRendererParams* common_renderer_params) {
  ZoomBy(zoom_delta, anchor,
         base::BindOnce(&SynchronousCompositorProxyChromeIPC::ZoomByReply,
                        base::Unretained(this), common_renderer_params));
}

void SynchronousCompositorProxyChromeIPC::OnSetScroll(
    const gfx::ScrollOffset& new_total_scroll_offset) {
  SetScroll(new_total_scroll_offset);
}

void SynchronousCompositorProxyChromeIPC::ZoomByReply(
    SyncCompositorCommonRendererParams* output_params,
    const SyncCompositorCommonRendererParams& params) {
  *output_params = params;
}

void SynchronousCompositorProxyChromeIPC::LayerTreeFrameSinkCreated() {
  Send(new SyncCompositorHostMsg_LayerTreeFrameSinkCreated(routing_id_));
}

}  // namespace content

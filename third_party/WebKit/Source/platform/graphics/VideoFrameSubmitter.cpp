// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/VideoFrameSubmitter.h"

#include "cc/base/filter_operations.h"
#include "cc/scheduler/video_frame_controller.h"
#include "components/viz/common/surfaces/local_surface_id_allocator.h"
#include "media/base/video_frame.h"
#include "platform/WebTaskRunner.h"
#include "platform/scheduler/child/web_scheduler.h"
#include "public/platform/InterfaceProvider.h"
#include "public/platform/Platform.h"
#include "public/platform/modules/offscreencanvas/offscreen_canvas_surface.mojom-blink.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom-blink.h"

namespace blink {

VideoFrameSubmitter::VideoFrameSubmitter(cc::VideoFrameProvider* provider,
                                         const viz::FrameSinkId& id)
    : provider_(provider),
      binding_(this),
      frame_sink_id_(id),
      rendering_(false) {
  DCHECK(frame_sink_id_.is_valid());

  current_begin_frame_ack_.source_id = viz::BeginFrameArgs::kManualSourceId;
  current_begin_frame_ack_.sequence_number =
      viz::BeginFrameArgs::kStartingFrameNumber;
  current_begin_frame_ack_.has_damage = true;

  current_local_surface_id_ = local_surface_id_allocator_.GenerateId();

  // Class to be renamed.
  mojom::blink::OffscreenCanvasProviderPtr canvas_provider;
  Platform::Current()->GetInterfaceProvider()->GetInterface(
      mojo::MakeRequest(&canvas_provider));

  scoped_refptr<base::SingleThreadTaskRunner> task_runner;
  viz::mojom::blink::CompositorFrameSinkClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client), task_runner);
  canvas_provider->CreateCompositorFrameSink(
      frame_sink_id_, std::move(client),
      mojo::MakeRequest(&compositor_frame_sink_));

  // This only happens during a commit on the media thread while the
  // main thread is blocked. That makes this a thread-safe call to set
  // the video frame provider client that does not require a lock.
  provider_->SetVideoFrameProviderClient(this);
}

VideoFrameSubmitter::~VideoFrameSubmitter() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void VideoFrameSubmitter::StopUsingProvider() {
  provider_ = nullptr;
  if (rendering_)
    StopRendering();
}

void VideoFrameSubmitter::StopRendering() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(rendering_);
  SubmitFrame();
  rendering_ = false;
  compositor_frame_sink_->SetNeedsBeginFrame(false);
}

void VideoFrameSubmitter::DidReceiveFrame() {
  DCHECK(thread_checker_.CalledOnValidThread());
  // TODO(lethalantidote): We do not want to submit double the frames,
  // but we also don't want to miss a frame. Waiting on further discussion.
  if (!rendering_)
    SubmitFrame();
}

void VideoFrameSubmitter::StartRendering() {
  DCHECK(!rendering_);
  compositor_frame_sink_->SetNeedsBeginFrame(true);
  rendering_ = true;
}

void VideoFrameSubmitter::SubmitFrame() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(compositor_frame_sink_);
  if (!provider_)
    return;

  cc::CompositorFrame compositor_frame;
  scoped_refptr<media::VideoFrame> video_frame = provider_->GetCurrentFrame();

  std::unique_ptr<cc::RenderPass> render_pass = cc::RenderPass::Create();
  gfx::Size viewport_size(10000, 10000);
  render_pass->SetNew(50, gfx::Rect(viewport_size), gfx::Rect(viewport_size),
                      gfx::Transform());
  render_pass->filters = cc::FilterOperations();
  resource_provider_.AppendQuads(render_pass.get());
  compositor_frame.render_pass_list.push_back(std::move(render_pass));
  compositor_frame.metadata.begin_frame_ack.has_damage = true;
  compositor_frame.metadata.begin_frame_ack = current_begin_frame_ack_;
  compositor_frame.metadata.device_scale_factor = 1;
  compositor_frame.metadata.may_contain_video = true;

  compositor_frame_sink_->SubmitCompositorFrame(current_local_surface_id_,
                                                std::move(compositor_frame));
  provider_->PutCurrentFrame();
}

void VideoFrameSubmitter::OnBeginFrame(const viz::BeginFrameArgs& args) {
  DCHECK(thread_checker_.CalledOnValidThread());

  current_begin_frame_ack_ =
      viz::BeginFrameAck(args.source_id, args.sequence_number, false);
  if (args.type == viz::BeginFrameArgs::MISSED) {
    compositor_frame_sink_->DidNotProduceFrame(current_begin_frame_ack_);
    return;
  }

  current_begin_frame_ack_.has_damage = true;
  begin_frame_args_ = args;

  if (!provider_ ||
      !provider_->UpdateCurrentFrame(args.frame_time + args.interval,
                                     args.frame_time + 2 * args.interval) ||
      !rendering_) {
    compositor_frame_sink_->DidNotProduceFrame(current_begin_frame_ack_);
    return;
  }

  SubmitFrame();
}

void VideoFrameSubmitter::DidReceiveCompositorFrameAck(
    const WTF::Vector<viz::ReturnedResource>& resources) {}

}  // namespace blink

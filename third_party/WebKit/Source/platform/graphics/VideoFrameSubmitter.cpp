// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/VideoFrameSubmitter.h"

#include "cc/base/filter_operations.h"
#include "cc/scheduler/video_frame_controller.h"
#include "components/viz/common/surfaces/local_surface_id_allocator.h"
#include "media/base/video_frame.h"

namespace blink {

VideoFrameSubmitter::VideoFrameSubmitter(cc::VideoFrameProvider* provider)
    : provider_(provider), binding_(this), rendering_(false) {
  current_local_surface_id_ = local_surface_id_allocator_.GenerateId();

  // This only happens during a commit on the media thread while the
  // main thread is blocked. That makes this a thread-safe call to set
  // the video frame provider client that does not require a lock.
  provider_->SetVideoFrameProviderClient(this);
}

VideoFrameSubmitter::~VideoFrameSubmitter() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (provider_)
    provider_->SetVideoFrameProviderClient(nullptr);
}

void VideoFrameSubmitter::StopUsingProvider() {
  if (rendering_)
    StopRendering();
  provider_ = nullptr;
}

void VideoFrameSubmitter::StopRendering() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(rendering_);
  viz::BeginFrameAck current_begin_frame_ack =
      viz::BeginFrameAck::CreateManualAckWithDamage();
  SubmitFrame(current_begin_frame_ack);
  rendering_ = false;
  compositor_frame_sink_->SetNeedsBeginFrame(false);
}

void VideoFrameSubmitter::DidReceiveFrame() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!rendering_) {
    viz::BeginFrameAck current_begin_frame_ack =
        viz::BeginFrameAck::CreateManualAckWithDamage();
    SubmitFrame(current_begin_frame_ack);
  }
}

void VideoFrameSubmitter::StartRendering() {
  DCHECK(!rendering_);
  compositor_frame_sink_->SetNeedsBeginFrame(true);
  rendering_ = true;
}

void VideoFrameSubmitter::SubmitFrame(viz::BeginFrameAck begin_frame_ack) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(compositor_frame_sink_);
  if (!provider_)
    return;

  cc::CompositorFrame compositor_frame;
  scoped_refptr<media::VideoFrame> video_frame = provider_->GetCurrentFrame();

  std::unique_ptr<cc::RenderPass> render_pass = cc::RenderPass::Create();

  // TODO(lethalantidote): Replace with true size. Current is just for test.
  gfx::Size viewport_size(10000, 10000);
  render_pass->SetNew(50, gfx::Rect(viewport_size), gfx::Rect(viewport_size),
                      gfx::Transform());
  render_pass->filters = cc::FilterOperations();
  resource_provider_.AppendQuads(*render_pass);
  compositor_frame.render_pass_list.push_back(std::move(render_pass));
  compositor_frame.metadata.begin_frame_ack = begin_frame_ack;
  compositor_frame.metadata.device_scale_factor = 1;
  compositor_frame.metadata.may_contain_video = true;

  // TODO(lethalantidote): Address third/fourth arg in SubmitCompositorFrame.
  compositor_frame_sink_->SubmitCompositorFrame(
      current_local_surface_id_, std::move(compositor_frame), nullptr, 0);
  provider_->PutCurrentFrame();
}

void VideoFrameSubmitter::OnBeginFrame(const viz::BeginFrameArgs& args) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  viz::BeginFrameAck current_begin_frame_ack =
      viz::BeginFrameAck(args.source_id, args.sequence_number, false);
  if (args.type == viz::BeginFrameArgs::MISSED) {
    compositor_frame_sink_->DidNotProduceFrame(current_begin_frame_ack);
    return;
  }

  current_begin_frame_ack.has_damage = true;

  if (!provider_ ||
      !provider_->UpdateCurrentFrame(args.frame_time + args.interval,
                                     args.frame_time + 2 * args.interval) ||
      !rendering_) {
    compositor_frame_sink_->DidNotProduceFrame(current_begin_frame_ack);
    return;
  }

  SubmitFrame(current_begin_frame_ack);
}

void VideoFrameSubmitter::DidReceiveCompositorFrameAck(
    const WTF::Vector<viz::ReturnedResource>& resources) {}

}  // namespace blink

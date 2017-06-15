// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/surface_tree_host.h"

#include <algorithm>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "cc/output/compositor_frame.h"
#include "cc/output/compositor_frame_sink.h"
#include "components/exo/compositor_frame_sink_holder.h"
#include "components/exo/surface.h"
#include "components/exo/surface_tree_host_delegate.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_targeter.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/class_property.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/path.h"

namespace exo {

////////////////////////////////////////////////////////////////////////////////
// SurfaceTreeHost, public:

SurfaceTreeHost::SurfaceTreeHost(SurfaceTreeHostDelegate* delegate)
    : delegate_(delegate) {
  window_ = base::MakeUnique<aura::Window>(nullptr);
  window_->SetType(aura::client::WINDOW_TYPE_CONTROL);
  window_->SetName("ExoSurfaceTreeHost");
  window_->Init(ui::LAYER_SOLID_COLOR);
  window_->set_owned_by_parent(false);
  window_->Show();
  window_->AddObserver(this);
  aura::Env::GetInstance()->context_factory()->AddObserver(this);
  compositor_frame_sink_holder_ = base::MakeUnique<CompositorFrameSinkHolder>(
      this, window_->CreateCompositorFrameSink());
}

SurfaceTreeHost::~SurfaceTreeHost() {
  aura::Env::GetInstance()->context_factory()->RemoveObserver(this);
  if (window_->layer()->GetCompositor())
    window_->layer()->GetCompositor()->vsync_manager()->RemoveObserver(this);
  if (surface_)
    DetachSurface();
}

void SurfaceTreeHost::AttachSurface(Surface* surface) {
  DCHECK(!surface_);
  DCHECK(surface);
  surface_ = surface;
  surface_->SetSurfaceDelegate(this);
  surface_->AddSurfaceObserver(this);
  window_->AddChild(surface_->window());
}

void SurfaceTreeHost::DetachSurface() {
  DCHECK(surface_);
  window_->RemoveChild(surface_->window());
  surface_->SetSurfaceDelegate(nullptr);
  surface_->RemoveSurfaceObserver(this);
  surface_ = nullptr;

  // Call all frame callbacks with a null frame time to indicate that they
  // have been cancelled.
  for (const auto& frame_callback : frame_callbacks_)
    frame_callback.Run(base::TimeTicks());

  swapping_presentation_callbacks_.splice(
      swapping_presentation_callbacks_.end(), presentation_callbacks_);
  swapped_presentation_callbacks_.splice(swapped_presentation_callbacks_.end(),
                                         swapping_presentation_callbacks_);

  // Call all presentation callbacks with a null presentation time to indicate
  // that they have been cancelled.
  for (const auto& presentation_callback : swapped_presentation_callbacks_)
    presentation_callback.Run(base::TimeTicks(), base::TimeDelta());
}

void SurfaceTreeHost::DidReceiveCompositorFrameAck() {
  active_frame_callbacks_.splice(active_frame_callbacks_.end(),
                                 frame_callbacks_);
  swapping_presentation_callbacks_.splice(
      swapping_presentation_callbacks_.end(), presentation_callbacks_);
  UpdateNeedsBeginFrame();
}

void SurfaceTreeHost::SetBeginFrameSource(
    cc::BeginFrameSource* begin_frame_source) {
  if (needs_begin_frame_) {
    DCHECK(begin_frame_source_);
    begin_frame_source_->RemoveObserver(this);
    needs_begin_frame_ = false;
  }
  begin_frame_source_ = begin_frame_source;
  UpdateNeedsBeginFrame();
}

void SurfaceTreeHost::UpdateNeedsBeginFrame() {
  if (!begin_frame_source_)
    return;

  bool needs_begin_frame = !active_frame_callbacks_.empty();
  if (needs_begin_frame == needs_begin_frame_)
    return;

  needs_begin_frame_ = needs_begin_frame;
  if (needs_begin_frame_)
    begin_frame_source_->AddObserver(this);
  else
    begin_frame_source_->RemoveObserver(this);
}

////////////////////////////////////////////////////////////////////////////////
// SurfaceDelegate overrides:

void SurfaceTreeHost::OnSurfaceCommit() {
  DCHECK(surface_);
  cc::CompositorFrame frame;
  // If we commit while we don't have an active BeginFrame, we acknowledge a
  // manual one.
  if (current_begin_frame_ack_.sequence_number ==
      cc::BeginFrameArgs::kInvalidFrameNumber) {
    current_begin_frame_ack_ = cc::BeginFrameAck::CreateManualAckWithDamage();
  } else {
    current_begin_frame_ack_.has_damage = true;
  }
  frame.metadata.begin_frame_ack = current_begin_frame_ack_;
  frame.metadata.device_scale_factor = device_scale_factor_;

  const int kRenderPassId = 1;
  std::unique_ptr<cc::RenderPass> render_pass = cc::RenderPass::Create();
  render_pass->SetNew(kRenderPassId, gfx::Rect(), gfx::Rect(),
                      gfx::Transform());
  frame.render_pass_list.push_back(std::move(render_pass));

  surface_->CommitSurfaceHierarchy(
      gfx::Point(0, 0), &frame, compositor_frame_sink_holder_.get(),
      &frame_callbacks_, &presentation_callbacks_, true);

  frame.render_pass_list.back()->output_rect =
      gfx::Rect(surface_->content_size());
  frame.render_pass_list.back()->damage_rect =
      gfx::Rect(surface_->content_size());

  compositor_frame_sink_holder_->GetCompositorFrameSink()
      ->SubmitCompositorFrame(std::move(frame));
  delegate_->OnSurfaceCommit();
}

bool SurfaceTreeHost::IsSurfaceSynchronized() const {
  DCHECK(surface_);
  return delegate_->IsSurfaceSynchronized();
}

////////////////////////////////////////////////////////////////////////////////
// SurfaceObserver overrides:

void SurfaceTreeHost::OnSurfaceDestroying(Surface* surface) {
  DetachSurface();
}

////////////////////////////////////////////////////////////////////////////////
// aura::WindowObserver overrides:

void SurfaceTreeHost::OnWindowAddedToRootWindow(aura::Window* window) {
  window->layer()->GetCompositor()->vsync_manager()->AddObserver(this);
}

void SurfaceTreeHost::OnWindowRemovingFromRootWindow(aura::Window* window,
                                                     aura::Window* new_root) {
  window->layer()->GetCompositor()->vsync_manager()->RemoveObserver(this);
}

void SurfaceTreeHost::OnWindowDestroying(aura::Window* window) {
  window->RemoveObserver(this);
}

////////////////////////////////////////////////////////////////////////////////
// cc::BeginFrameObserverBase overrides:

bool SurfaceTreeHost::OnBeginFrameDerivedImpl(const cc::BeginFrameArgs& args) {
  current_begin_frame_ack_ = cc::BeginFrameAck(
      args.source_id, args.sequence_number, args.sequence_number, false);
  while (!active_frame_callbacks_.empty()) {
    active_frame_callbacks_.front().Run(args.frame_time);
    active_frame_callbacks_.pop_front();
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// ui::CompositorVSyncManager::Observer overrides:

void SurfaceTreeHost::OnUpdateVSyncParameters(base::TimeTicks timebase,
                                              base::TimeDelta interval) {
  // Use current time if platform doesn't provide an accurate timebase.
  if (timebase.is_null())
    timebase = base::TimeTicks::Now();

  while (!swapped_presentation_callbacks_.empty()) {
    swapped_presentation_callbacks_.front().Run(timebase, interval);
    swapped_presentation_callbacks_.pop_front();
  }

  // VSync parameters updates are generated at the start of a new swap. Move
  // the swapping presentation callbacks to swapped callbacks so they fire
  // at the next VSync parameters update as that will contain the presentation
  // time for the previous frame.
  swapped_presentation_callbacks_.splice(swapped_presentation_callbacks_.end(),
                                         swapping_presentation_callbacks_);
}

////////////////////////////////////////////////////////////////////////////////
// ui::ContextFactoryObserver overrides:

void SurfaceTreeHost::OnLostResources() {
  if (!window_->GetSurfaceId().is_valid())
    return;
  surface_->OnLostResources(compositor_frame_sink_holder_.get());
}

}  // namespace exo

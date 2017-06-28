// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/surface_tree_host.h"

#include <algorithm>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "components/exo/surface.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_targeter.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/cursor/cursor.h"
#include "ui/gfx/path.h"

namespace exo {

namespace {

class CustomWindowTargeter : public aura::WindowTargeter {
 public:
  explicit CustomWindowTargeter(SurfaceTreeHost* surface_tree_host)
      : surface_tree_host_(surface_tree_host) {}
  ~CustomWindowTargeter() override = default;

  // Overridden from aura::WindowTargeter:
  bool EventLocationInsideBounds(aura::Window* window,
                                 const ui::LocatedEvent& event) const override {
    if (window != surface_tree_host_->host_window())
      return false;

    Surface* surface = surface_tree_host_->root_surface();
    if (!surface)
      return false;

    gfx::Point local_point = event.location();

    if (window->parent())
      aura::Window::ConvertPointToTarget(window->parent(), window,
                                         &local_point);
    aura::Window::ConvertPointToTarget(window, surface->window(), &local_point);
    return surface->HitTestRect(gfx::Rect(local_point, gfx::Size(1, 1)));
  }

  ui::EventTarget* FindTargetForEvent(ui::EventTarget* root,
                                      ui::Event* event) override {
    aura::Window* window = static_cast<aura::Window*>(root);
    DCHECK_EQ(window, surface_tree_host_->host_window());
    ui::EventTarget* target =
        aura::WindowTargeter::FindTargetForEvent(root, event);
    // Do not accept events in SurfaceTreeHost window.
    return target != root ? target : nullptr;
  }

 private:
  SurfaceTreeHost* const surface_tree_host_;

  DISALLOW_COPY_AND_ASSIGN(CustomWindowTargeter);
};

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// SurfaceTreeHost, public:

SurfaceTreeHost::SurfaceTreeHost(const std::string& window_name,
                                 aura::WindowDelegate* window_delegate) {
  host_window_ = base::MakeUnique<aura::Window>(window_delegate);
  host_window_->SetType(aura::client::WINDOW_TYPE_CONTROL);
  host_window_->SetName(window_name);
  host_window_->Init(ui::LAYER_NOT_DRAWN);
  host_window_->set_owned_by_parent(false);
  host_window_->SetEventTargeter(base::MakeUnique<CustomWindowTargeter>(this));
}

SurfaceTreeHost::~SurfaceTreeHost() {
  if (root_surface_) {
    root_surface_->window()->Hide();
    root_surface_->SetSurfaceDelegate(nullptr);
  }
}

void SurfaceTreeHost::SetRootSurface(Surface* root_surface) {
  if (root_surface == root_surface_)
    return;

  if (root_surface_) {
    root_surface_->window()->Hide();
    host_window_->RemoveChild(root_surface_->window());
    host_window_->SetBounds(
        gfx::Rect(host_window_->bounds().origin(), gfx::Size()));
    root_surface_->SetSurfaceDelegate(nullptr);
    root_surface_ = nullptr;
  }

  if (root_surface) {
    root_surface_ = root_surface;
    root_surface_->SetSurfaceDelegate(this);
    host_window_->AddChild(root_surface_->window());
    host_window_->SetBounds(gfx::Rect(host_window_->bounds().origin(),
                                      root_surface_->content_size()));
    root_surface_->window()->Show();
  }
}

bool SurfaceTreeHost::HasHitTestMask() const {
  return root_surface_ ? root_surface_->HasHitTestMask() : false;
}

void SurfaceTreeHost::GetHitTestMask(gfx::Path* mask) const {
  if (root_surface_)
    root_surface_->GetHitTestMask(mask);
}

gfx::Rect SurfaceTreeHost::GetHitTestBounds() const {
  return root_surface_ ? root_surface_->GetHitTestBounds() : gfx::Rect();
}

gfx::NativeCursor SurfaceTreeHost::GetCursor(const gfx::Point& point) const {
  return root_surface_ ? root_surface_->GetCursor() : ui::CursorType::kNull;
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
  DCHECK(root_surface_);
  root_surface_->CommitSurfaceHierarchy();
  host_window_->SetBounds(gfx::Rect(host_window_->bounds().origin(),
                                    root_surface_->content_size()));
}

bool SurfaceTreeHost::IsSurfaceSynchronized() const {
  // To host a surface tree, the root surface has to be desynchronized.
  DCHECK(root_surface_);
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// aura::WindowObserver overrides:

void SurfaceTreeHost::OnWindowAddedToRootWindow(aura::Window* window) {
  DCHECK_EQ(window, host_window());
  window->layer()->GetCompositor()->vsync_manager()->AddObserver(this);
}

void SurfaceTreeHost::OnWindowRemovingFromRootWindow(aura::Window* window,
                                                     aura::Window* new_root) {
  DCHECK_EQ(window, host_window());
  window->layer()->GetCompositor()->vsync_manager()->RemoveObserver(this);
}

void SurfaceTreeHost::OnWindowDestroying(aura::Window* window) {
  DCHECK_EQ(window, host_window());
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
  if (!host_window_->GetSurfaceId().is_valid())
    return;
  // root_surface_->OnLostResources(compositor_frame_sink_holder_.get());
}

}  // namespace exo

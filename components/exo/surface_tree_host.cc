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
    if (window != surface_tree_host_->window())
      return false;

    Surface* surface = surface_tree_host_->surface();
    if (!surface)
      return false;

    gfx::Point local_point = event.location();

    if (window->parent())
      aura::Window::ConvertPointToTarget(window->parent(), window,
                                         &local_point);

    // Otherwise, fallback to hit test on the surface.
    aura::Window::ConvertPointToTarget(window, surface->window(), &local_point);
    return surface->HitTestRect(gfx::Rect(local_point, gfx::Size(1, 1)));
  }

  ui::EventTarget* FindTargetForEvent(ui::EventTarget* root,
                                      ui::Event* event) override {
    aura::Window* window = static_cast<aura::Window*>(root);
    DCHECK_EQ(window, surface_tree_host_->window());
    ui::EventTarget* target =
        aura::WindowTargeter::FindTargetForEvent(root, event);
    // Do not accept events in ShellSurface window.
    return target != root ? target : nullptr;
  }

 private:
  SurfaceTreeHost* const surface_tree_host_;

  DISALLOW_COPY_AND_ASSIGN(CustomWindowTargeter);
};

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// SurfaceTreeHost, public:

SurfaceTreeHost::SurfaceTreeHost(aura::WindowDelegate* window_delegate) {
  window_ = base::MakeUnique<aura::Window>(window_delegate);
  window_->SetType(aura::client::WINDOW_TYPE_CONTROL);
  window_->SetName("ExoSurfaceTreeHost");
  window_->Init(ui::LAYER_NOT_DRAWN);
  window_->set_owned_by_parent(false);
  window_->SetEventTargeter(base::MakeUnique<CustomWindowTargeter>(this));
}

SurfaceTreeHost::~SurfaceTreeHost() {
  if (surface_)
    DetachSurfaceInternal(true);
}

void SurfaceTreeHost::AttachSurface(Surface* surface) {
  DCHECK(!surface_);
  DCHECK(surface);
  surface_ = surface;
  surface_->SetSurfaceDelegate(this);
  surface_->AddSurfaceObserver(this);
  window_->AddChild(surface_->window());
  window_->SetBounds(
      gfx::Rect(window_->bounds().origin(), surface_->content_size()));
  surface_->window()->Show();
}

void SurfaceTreeHost::DetachSurface() {
  DetachSurfaceInternal(false);
}

bool SurfaceTreeHost::HasHitTestMask() const {
  return surface_ ? surface_->HasHitTestMask() : false;
}

void SurfaceTreeHost::GetHitTestMask(gfx::Path* mask) const {
  // TODO should we combine  hit test mask for all subsurfaces?
  if (surface_)
    surface_->GetHitTestMask(mask);
}

gfx::Rect SurfaceTreeHost::GetHitTestBounds() const {
  // TODO should we combine input regions of all subsurfaces?
  return surface_ ? surface_->GetHitTestBounds() : gfx::Rect();
}

gfx::NativeCursor SurfaceTreeHost::GetCursor(const gfx::Point& point) const {
  // TODO return the of a sub surface which is under the point.
  return surface_ ? surface_->GetCursor() : ui::CursorType::kNull;
}

////////////////////////////////////////////////////////////////////////////////
// SurfaceDelegate overrides:

void SurfaceTreeHost::OnSurfaceCommit() {
  DCHECK(surface_);
  surface_->CommitSurfaceHierarchy();
  window_->SetBounds(
      gfx::Rect(window_->bounds().origin(), surface_->content_size()));
  OnSurfaceTreeCommitted();
}

bool SurfaceTreeHost::IsSurfaceSynchronized() const {
  // To host a surface tree, the root surface has to be desynchronized.
  DCHECK(surface_);
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// SurfaceObserver overrides:

void SurfaceTreeHost::OnSurfaceDestroying(Surface* surface) {
  DCHECK_EQ(surface, surface_);
  DetachSurfaceInternal(false);
}

////////////////////////////////////////////////////////////////////////////////
// SurfaceTreeHost, private:

void SurfaceTreeHost::DetachSurfaceInternal(bool in_destructor) {
  DCHECK(surface_);
  surface_->window()->Hide();
  window_->RemoveChild(surface_->window());
  window_->SetBounds(gfx::Rect(window_->bounds().origin(), gfx::Size()));
  surface_->SetSurfaceDelegate(nullptr);
  surface_->RemoveSurfaceObserver(this);
  surface_ = nullptr;

  // Should not call virtual method in destructor.
  if (!in_destructor)
    OnSurfaceDetached();
}

}  // namespace exo

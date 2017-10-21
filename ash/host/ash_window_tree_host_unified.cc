// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/host/ash_window_tree_host_unified.h"

#include <memory>
#include <utility>

#include "ash/display/mirror_window_controller.h"
#include "ash/display/screen_position_controller.h"
#include "ash/display/window_tree_host_manager.h"
#include "ash/host/root_window_transformer.h"
#include "ash/shell.h"
#include "base/logging.h"
#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_targeter.h"
#include "ui/compositor/compositor.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/platform_window/stub/stub_window.h"

namespace ash {

namespace {

class UnifiedEventTargeter : public aura::WindowTargeter {
 public:
  UnifiedEventTargeter(aura::Window* src_root, aura::Window* dst_root)
      : src_root_(src_root), dst_root_(dst_root) {}

  ui::EventTarget* FindTargetForEvent(ui::EventTarget* root,
                                      ui::Event* event) override {
    if (root == src_root_ && !event->target()) {
      Shell::Get()
          ->window_tree_host_manager()
          ->mirror_window_controller()
          ->set_current_event_targeter_src_host(src_root_->GetHost());

      if (event->IsLocatedEvent()) {
        ui::LocatedEvent* located_event = static_cast<ui::LocatedEvent*>(event);
        located_event->ConvertLocationToTarget(
            static_cast<aura::Window*>(nullptr), dst_root_);
      }
      ignore_result(
          dst_root_->GetHost()->event_sink()->OnEventFromSource(event));

      // Reset the source host.
      Shell::Get()
          ->window_tree_host_manager()
          ->mirror_window_controller()
          ->set_current_event_targeter_src_host(nullptr);

      return nullptr;
    } else {
      NOTREACHED() << "event type:" << event->type();
      return aura::WindowTargeter::FindTargetForEvent(root, event);
    }
  }

 private:
  aura::Window* src_root_;
  aura::Window* dst_root_;

  DISALLOW_COPY_AND_ASSIGN(UnifiedEventTargeter);
};

// ScreenPositionClient for the unified mirroring hosts windows.
class UnifiedMirroringScreenPositionClient
    : public aura::client::ScreenPositionClient {
 public:
  UnifiedMirroringScreenPositionClient() = default;
  ~UnifiedMirroringScreenPositionClient() override = default;

  void ConvertPointToScreen(const aura::Window* window,
                            gfx::PointF* point) override {
    const aura::Window* root = window->GetRootWindow();
    aura::Window::ConvertPointToTarget(window, root, point);
  }

  void ConvertPointFromScreen(const aura::Window* window,
                              gfx::PointF* point) override {
    const aura::Window* root = window->GetRootWindow();
    aura::Window::ConvertPointToTarget(root, window, point);
  }

  void ConvertHostPointToScreen(aura::Window* root_window,
                                gfx::Point* point) override {
    aura::Window* not_used;
    MirrorWindowController* controller =
        Shell::Get()->window_tree_host_manager()->mirror_window_controller();
    ScreenPositionController::ConvertHostPointToRelativeToRootWindow(
        root_window, controller->GetAllRootWindows(), point, &not_used);
    aura::client::ScreenPositionClient::ConvertPointToScreen(root_window,
                                                             point);
  }

  void SetBounds(aura::Window* window,
                 const gfx::Rect& bounds,
                 const display::Display& display) override {
    NOTREACHED();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(UnifiedMirroringScreenPositionClient);
};

}  // namespace

AshWindowTreeHostUnified::AshWindowTreeHostUnified(
    const gfx::Rect& initial_bounds)
    : AshWindowTreeHostPlatform(),
      screen_position_client_(new UnifiedMirroringScreenPositionClient) {
  std::unique_ptr<ui::PlatformWindow> window(new ui::StubWindow(this));
  window->SetBounds(initial_bounds);
  SetPlatformWindow(std::move(window));
}

AshWindowTreeHostUnified::~AshWindowTreeHostUnified() {
  for (auto* ash_host : mirroring_hosts_)
    ash_host->AsWindowTreeHost()->window()->RemoveObserver(this);
}

void AshWindowTreeHostUnified::PrepareForShutdown() {
  AshWindowTreeHostPlatform::PrepareForShutdown();

  for (auto* host : mirroring_hosts_)
    host->PrepareForShutdown();
}

void AshWindowTreeHostUnified::RegisterMirroringHost(
    AshWindowTreeHost* mirroring_ash_host) {
  aura::Window* src_root = mirroring_ash_host->AsWindowTreeHost()->window();
  aura::client::SetScreenPositionClient(src_root,
                                        screen_position_client_.get());
  src_root->SetEventTargeter(
      std::make_unique<UnifiedEventTargeter>(src_root, window()));
  DCHECK(std::find(mirroring_hosts_.begin(), mirroring_hosts_.end(),
                   mirroring_ash_host) == mirroring_hosts_.end());
  mirroring_hosts_.push_back(mirroring_ash_host);
  mirroring_ash_host->AsWindowTreeHost()->window()->AddObserver(this);
}

void AshWindowTreeHostUnified::SetBoundsInPixels(const gfx::Rect& bounds) {
  AshWindowTreeHostPlatform::SetBoundsInPixels(bounds);
  OnHostResizedInPixels(bounds.size());
}

void AshWindowTreeHostUnified::SetCursorNative(gfx::NativeCursor cursor) {
  for (auto* host : mirroring_hosts_)
    host->AsWindowTreeHost()->SetCursor(cursor);
}

void AshWindowTreeHostUnified::OnCursorVisibilityChangedNative(bool show) {
  for (auto* host : mirroring_hosts_)
    host->AsWindowTreeHost()->OnCursorVisibilityChanged(show);
}

void AshWindowTreeHostUnified::OnBoundsChanged(const gfx::Rect& bounds) {
  if (platform_window())
    OnHostResizedInPixels(bounds.size());
}

void AshWindowTreeHostUnified::OnWindowDestroying(aura::Window* window) {
  auto iter =
      std::find_if(mirroring_hosts_.begin(), mirroring_hosts_.end(),
                   [window](AshWindowTreeHost* ash_host) {
                     return ash_host->AsWindowTreeHost()->window() == window;
                   });
  DCHECK(iter != mirroring_hosts_.end());
  window->RemoveObserver(this);
  mirroring_hosts_.erase(iter);
}

}  // namespace ash

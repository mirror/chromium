// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/shell_surface_widget_wrapper.h"

#include "ash/public/cpp/window_properties.h"
#include "ash/wm/client_controlled_state.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_state_delegate.h"
#include "components/exo/shell_surface.h"
#include "ui/views/widget/widget.h"

namespace exo {

namespace {

class CustomClientControlledStateDelegate
    : public ash::wm::ClientControlledState::Delegate {
 public:
  CustomClientControlledStateDelegate(ShellSurface* shell_surface)
      : shell_surface_(shell_surface) {}
  ~CustomClientControlledStateDelegate() override {}
  void SendWindowStateRequest(ash::mojom::WindowStateType current_state,
                              ash::mojom::WindowStateType next_state) override {
    shell_surface_->SendWindowStateRequest(current_state, next_state);
  }

  void SendBoundsRequest(ash::mojom::WindowStateType current_state,
                         const gfx::Rect& bounds) override {
    shell_surface_->SendBoundsRequest(current_state, bounds);
  }

 private:
  ShellSurface* shell_surface_;
  DISALLOW_COPY_AND_ASSIGN(CustomClientControlledStateDelegate);
};

class CustomWindowStateDelegate : public ash::wm::WindowStateDelegate {
 public:
  explicit CustomWindowStateDelegate(ShellSurface* shell_surface)
      : shell_surface_(shell_surface) {}
  ~CustomWindowStateDelegate() override {}

  bool ToggleFullscreen(ash::wm::WindowState* window_state) override {
    return shell_surface_->ToggleFullscreen(window_state->GetStateType());
  }

  bool RestoreAlwaysOnTop(ash::wm::WindowState* window_state) override {
    return false;
  }

 private:
  ShellSurface* shell_surface_;
  DISALLOW_COPY_AND_ASSIGN(CustomWindowStateDelegate);
};

class ClientControlledWidgetWrapper : public ShellSurfaceWidgetWrapper {
 public:
  explicit ClientControlledWidgetWrapper(views::Widget* widget)
      : ShellSurfaceWidgetWrapper(widget) {}

  void Init(ShellSurface* shell_surface) override {
    ash::wm::WindowState* window_state = GetWindowState();
    std::unique_ptr<ash::wm::ClientControlledState> state =
        std::make_unique<ash::wm::ClientControlledState>(
            std::make_unique<CustomClientControlledStateDelegate>(
                shell_surface));
    client_controlled_state_ = state.get();
    window_state->SetStateObject(std::move(state));
    window_state->SetDelegate(
        std::make_unique<CustomWindowStateDelegate>(shell_surface));
  }

  ~ClientControlledWidgetWrapper() override = default;

  void Restore() override {
    ash::wm::WindowState* window_state = GetWindowState();
    client_controlled_state_->EnterToNextState(
        window_state, ash::mojom::WindowStateType::NORMAL);
  }
  void Minimize() override {
    ash::wm::WindowState* window_state = GetWindowState();
    client_controlled_state_->EnterToNextState(
        window_state, ash::mojom::WindowStateType::MINIMIZED);
  }
  void Maximize() override {
    ash::wm::WindowState* window_state = GetWindowState();
    client_controlled_state_->EnterToNextState(
        window_state, ash::mojom::WindowStateType::MAXIMIZED);
  }
  void SetFullscreen(bool fullscreen) override {
    // For CLIENT mode, |fullscreen| is always true. Exiting fullscreen
    // is done via Restore.
    ash::wm::WindowState* window_state = GetWindowState();
    client_controlled_state_->EnterToNextState(
        window_state, ash::mojom::WindowStateType::FULLSCREEN);
  }
  void SetBounds(const gfx::Rect& bounds) override {
    client_controlled_state_->set_bounds_locally(true);
    widget()->SetBounds(bounds);
    client_controlled_state_->set_bounds_locally(false);
  }

  void SetBoundsInParent(const gfx::Rect& bounds) override {
    client_controlled_state_->set_bounds_locally(true);
    widget()->GetNativeWindow()->SetBounds(bounds);
    client_controlled_state_->set_bounds_locally(false);
  }

 private:
  ash::wm::ClientControlledState* client_controlled_state_ = nullptr;
  DISALLOW_COPY_AND_ASSIGN(ClientControlledWidgetWrapper);
};

}  // namespace

// static
std::unique_ptr<ShellSurfaceWidgetWrapper> ShellSurfaceWidgetWrapper::Create(
    ShellSurface::BoundsMode bounds_mode) {
  views::Widget* widget = new views::Widget();
  return bounds_mode == ShellSurface::BoundsMode::CLIENT
             ? std::make_unique<ClientControlledWidgetWrapper>(widget)
             : std::make_unique<ShellSurfaceWidgetWrapper>(widget);
}

ShellSurfaceWidgetWrapper::ShellSurfaceWidgetWrapper(views::Widget* widget)
    : widget_(widget) {}

ShellSurfaceWidgetWrapper::~ShellSurfaceWidgetWrapper() = default;

void ShellSurfaceWidgetWrapper::Init(ShellSurface* shell_surface) {}

void ShellSurfaceWidgetWrapper::Restore() {
  widget_->Restore();
}

void ShellSurfaceWidgetWrapper::Minimize() {
  widget_->Minimize();
}

void ShellSurfaceWidgetWrapper::Maximize() {
  widget_->Maximize();
}

void ShellSurfaceWidgetWrapper::SetFullscreen(bool fullscreen) {
  widget_->SetFullscreen(fullscreen);
}

void ShellSurfaceWidgetWrapper::SetPinned(ash::mojom::WindowPinType type) {
  widget_->GetNativeWindow()->SetProperty(ash::kWindowPinTypeKey, type);
}

void ShellSurfaceWidgetWrapper::SetBounds(const gfx::Rect& bounds) {
  widget_->SetBounds(bounds);
}

void ShellSurfaceWidgetWrapper::SetBoundsInParent(const gfx::Rect& bounds) {
  widget_->GetNativeWindow()->SetBounds(bounds);
}

gfx::NativeWindow ShellSurfaceWidgetWrapper::GetNativeWindow() {
  return widget_->GetNativeWindow();
}

gfx::Rect ShellSurfaceWidgetWrapper::GetBoundsForClientView() const {
  const views::NonClientView* non_client_view = widget_->non_client_view();
  return non_client_view->frame_view()->GetBoundsForClientView();
}

ash::wm::WindowState* ShellSurfaceWidgetWrapper::GetWindowState() {
  return ash::wm::GetWindowState(widget_->GetNativeWindow());
}

}  // namespace exo

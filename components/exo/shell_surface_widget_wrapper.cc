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

std::unique_ptr<ShellSurfaceWidgetWrapper::ClientControlledStateDelegateFactory>
    g_delegate_factory;

class ShellSurfaceWidget : public views::Widget {
 public:
  explicit ShellSurfaceWidget(ShellSurface* shell_surface)
      : shell_surface_(shell_surface) {}

  // Overridden from views::Widget
  void Close() override { shell_surface_->Close(); }
  void OnKeyEvent(ui::KeyEvent* event) override {
    // Handle only accelerators. Do not call Widget::OnKeyEvent that eats focus
    // management keys (like the tab key) as well.
    if (GetFocusManager()->ProcessAccelerator(ui::Accelerator(*event)))
      event->SetHandled();
  }

 private:
  ShellSurface* const shell_surface_;

  DISALLOW_COPY_AND_ASSIGN(ShellSurfaceWidget);
};

class CustomClientControlledStateDelegate
    : public ash::wm::ClientControlledState::Delegate {
 public:
  CustomClientControlledStateDelegate(ShellSurface* shell_surface)
      : shell_surface_(shell_surface) {}
  ~CustomClientControlledStateDelegate() override {}
  void HandleWindowStateRequest(
      ash::wm::WindowState* window_state,
      ash::wm::ClientControlledState* state,
      ash::mojom::WindowStateType next_state) override {
    shell_surface_->SendWindowStateRequest(window_state->GetStateType(),
                                           next_state);
  }

  void HandleBoundsRequest(ash::wm::WindowState* window_state,
                           ash::wm::ClientControlledState* state,
                           const gfx::Rect& bounds) override {
    shell_surface_->SendBoundsRequest(window_state->GetStateType(), bounds);
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
  ClientControlledWidgetWrapper(ShellSurface* shell_surface,
                                views::Widget* widget)
      : ShellSurfaceWidgetWrapper(shell_surface, widget) {}

  void Init() override {
    ShellSurfaceWidgetWrapper::Init();
    ash::wm::WindowState* window_state = GetWindowState();
    std::unique_ptr<ash::wm::ClientControlledState::Delegate> delegate =
        g_delegate_factory.get()
            ? g_delegate_factory->Create()
            : std::make_unique<CustomClientControlledStateDelegate>(
                  shell_surface_);
    std::unique_ptr<ash::wm::ClientControlledState> state =
        std::make_unique<ash::wm::ClientControlledState>(std::move(delegate));
    client_controlled_state_ = state.get();
    window_state->SetStateObject(std::move(state));
    window_state->SetDelegate(
        std::make_unique<CustomWindowStateDelegate>(shell_surface_));
  }

  ~ClientControlledWidgetWrapper() override = default;

  void SyncInitialState() override {}

  void Restore() override {
    ash::wm::WindowState* window_state = GetWindowState();
    client_controlled_state_->EnterNextState(
        window_state, ash::mojom::WindowStateType::NORMAL);
  }
  void Minimize() override {
    ash::wm::WindowState* window_state = GetWindowState();
    if (IsPinned(window_state))
      return;
    client_controlled_state_->EnterNextState(
        window_state, ash::mojom::WindowStateType::MINIMIZED);
  }
  void Maximize() override {
    ash::wm::WindowState* window_state = GetWindowState();
    if (IsPinned(window_state))
      return;
    client_controlled_state_->EnterNextState(
        window_state, ash::mojom::WindowStateType::MAXIMIZED);
  }
  void SetFullscreen(bool fullscreen) override {
    ash::wm::WindowState* window_state = GetWindowState();
    if (IsPinned(window_state))
      return;
    client_controlled_state_->EnterNextState(
        window_state, fullscreen ? ash::mojom::WindowStateType::FULLSCREEN
                                 : ash::mojom::WindowStateType::NORMAL);
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
  static bool IsPinned(const ash::wm::WindowState* window_state) {
    return window_state->IsPinned() || window_state->IsTrustedPinned();
  }

  ash::wm::ClientControlledState* client_controlled_state_ = nullptr;
  DISALLOW_COPY_AND_ASSIGN(ClientControlledWidgetWrapper);
};

}  // namespace

// static
std::unique_ptr<ShellSurfaceWidgetWrapper> ShellSurfaceWidgetWrapper::Create(
    ShellSurface* shell_surface) {
  views::Widget* widget = new ShellSurfaceWidget(shell_surface);
  return shell_surface->bounds_mode() == ShellSurface::BoundsMode::CLIENT
             ? std::make_unique<ClientControlledWidgetWrapper>(shell_surface,
                                                               widget)
             : std::make_unique<ShellSurfaceWidgetWrapper>(shell_surface,
                                                           widget);
}

ShellSurfaceWidgetWrapper::ShellSurfaceWidgetWrapper(
    ShellSurface* shell_surface,
    views::Widget* widget)
    : shell_surface_(shell_surface), widget_(widget) {}

ShellSurfaceWidgetWrapper::~ShellSurfaceWidgetWrapper() = default;

void ShellSurfaceWidgetWrapper::Init() {}

void ShellSurfaceWidgetWrapper::SyncInitialState() {
  ash::wm::WindowState* window_state = GetWindowState();
  if (window_state->GetStateType() != ash::mojom::WindowStateType::NORMAL) {
    shell_surface_->SendWindowStateRequest(ash::mojom::WindowStateType::NORMAL,
                                           window_state->GetStateType());
  }
}

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

// static
void ShellSurfaceWidgetWrapper::SetClientControlledStateDelegateFactory(
    std::unique_ptr<ClientControlledStateDelegateFactory> factory) {
  g_delegate_factory = std::move(factory);
}

}  // namespace exo

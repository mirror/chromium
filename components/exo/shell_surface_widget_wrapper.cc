// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/shell_surface_widget_wrapper.h"

#include "ash/public/cpp/window_properties.h"
#include "ash/public/interfaces/window_state_type.mojom.h"
#include "ash/wm/client_controlled_state.h"
#include "ash/wm/window_properties.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_state_delegate.h"
#include "ash/wm/window_resizer.h"
#include "components/exo/shell_surface.h"
#include "ui/aura/client/aura_constants.h"
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

// A ClientControlledStateDelegate that sends the state/bounds
// change request to exo client.
class ClientControlledStateDelegate
    : public ash::wm::ClientControlledState::Delegate {
 public:
  explicit ClientControlledStateDelegate(ShellSurface* shell_surface)
      : shell_surface_(shell_surface) {}
  ~ClientControlledStateDelegate() override {}

  // Overridden from ash::wm::ClientControlledState::Delegate
  void HandleWindowStateRequest(
      ash::wm::WindowState* window_state,
      ash::mojom::WindowStateType next_state) override {
    shell_surface_->SendWindowStateChangeEvent(window_state->GetStateType(),
                                               next_state);
  }
  void HandleBoundsRequest(ash::wm::WindowState* window_state,
                           const gfx::Rect& bounds) override {

    shell_surface_->SendBoundsChangeEvent(
        window_state->GetStateType(),
        bounds,
        window_state->is_dragged(),
        (!window_state->is_dragged() ? false :
         (window_state->drag_details()->bounds_change &
          ash::WindowResizer::kBoundsChange_Resizes) != 0));
    // TODO(oshima): Implement this.
  }

 private:
  ShellSurface* shell_surface_;
  DISALLOW_COPY_AND_ASSIGN(ClientControlledStateDelegate);
};

// A WindowStateDelegate that implements ToggleFullscreen behavior for
// client controlled window.
class ClientControlledWindowStateDelegate
    : public ash::wm::WindowStateDelegate {
 public:
  explicit ClientControlledWindowStateDelegate(
      ShellSurface* shell_surface,
      ash::wm::ClientControlledState::Delegate* delegate)
      : shell_surface_(shell_surface), delegate_(delegate) {}
  ~ClientControlledWindowStateDelegate() override {}

  // Overridden from ash::wm::WindowStateDelegate:
  bool ToggleFullscreen(ash::wm::WindowState* window_state) override {
    ash::mojom::WindowStateType next_state;
    // ash::mojom::WindowStateType current_state = window_state->GetStateType();
    aura::Window* window = window_state->window();
    switch (window_state->GetStateType()) {
      case ash::mojom::WindowStateType::DEFAULT:
      // current_state = ash::mojom::WindowStateType::NORMAL;
      case ash::mojom::WindowStateType::NORMAL:
        window->SetProperty(aura::client::kPreFullscreenShowStateKey,
                            ui::SHOW_STATE_NORMAL);
        next_state = ash::mojom::WindowStateType::FULLSCREEN;
        break;
      case ash::mojom::WindowStateType::MAXIMIZED:
        window->SetProperty(aura::client::kPreFullscreenShowStateKey,
                            ui::SHOW_STATE_MAXIMIZED);
        next_state = ash::mojom::WindowStateType::FULLSCREEN;
        break;
      case ash::mojom::WindowStateType::FULLSCREEN:
        switch (window->GetProperty(aura::client::kPreFullscreenShowStateKey)) {
          case ui::SHOW_STATE_DEFAULT:
          case ui::SHOW_STATE_NORMAL:
            next_state = ash::mojom::WindowStateType::NORMAL;
            break;
          case ui::SHOW_STATE_MAXIMIZED:
            next_state = ash::mojom::WindowStateType::MAXIMIZED;
            break;
          case ui::SHOW_STATE_MINIMIZED:
            next_state = ash::mojom::WindowStateType::MINIMIZED;
            break;
          case ui::SHOW_STATE_FULLSCREEN:
          case ui::SHOW_STATE_INACTIVE:
          case ui::SHOW_STATE_END:
            NOTREACHED() << " unknown state :"
                         << window->GetProperty(
                                aura::client::kPreFullscreenShowStateKey);
            return false;
        }
        break;
      case ash::mojom::WindowStateType::MINIMIZED: {
        ui::WindowShowState pre_full_state =
            window->GetProperty(aura::client::kPreMinimizedShowStateKey);
        if (pre_full_state != ui::SHOW_STATE_FULLSCREEN) {
          window->SetProperty(aura::client::kPreFullscreenShowStateKey,
                              pre_full_state);
        }
        next_state = ash::mojom::WindowStateType::FULLSCREEN;
        break;
      }
      default:
        // TODO(oshima|xdai): Handle SNAPstate.
        return false;
    }
    delegate_->HandleWindowStateRequest(window_state, next_state);
    return true;
  }

  void StartDrag(int component) override {
    shell_surface_->StartDrag(component);
  }
  void EndDrag() override {
    shell_surface_->EndDrag();
  }

 private:
  ShellSurface* shell_surface_;
  ash::wm::ClientControlledState::Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(ClientControlledWindowStateDelegate);
};

// A ShellSurfaceWidgetWrapper that delegates the state chagne request to
// exo client.
class ClientControlledWidgetWrapper : public ShellSurfaceWidgetWrapper {
 public:
  ClientControlledWidgetWrapper(ShellSurface* shell_surface,
                                views::Widget* widget)
      : ShellSurfaceWidgetWrapper(shell_surface, widget) {}
  ~ClientControlledWidgetWrapper() override = default;

  void Init() override {
    ShellSurfaceWidgetWrapper::Init();
    ash::wm::WindowState* window_state = GetWindowState();
    std::unique_ptr<ash::wm::ClientControlledState::Delegate> delegate =
        g_delegate_factory.get()
            ? g_delegate_factory->Create()
            : std::make_unique<ClientControlledStateDelegate>(shell_surface());
    auto window_delegate =
        std::make_unique<ClientControlledWindowStateDelegate>(shell_surface(),
                                                              delegate.get());
    auto state =
        std::make_unique<ash::wm::ClientControlledState>(std::move(delegate));
    client_controlled_state_ = state.get();
    window_state->SetStateObject(std::move(state));
    window_state->SetDelegate(std::move(window_delegate));
  }

  // Overridden from ShellSurfaceWidgetWrapper:
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
  ash::mojom::WindowStateType initial_state = GetWindowState()->GetStateType();
  if (initial_state != ash::mojom::WindowStateType::NORMAL) {
    shell_surface_->SendWindowStateChangeEvent(
        ash::mojom::WindowStateType::NORMAL, initial_state);
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

ash::wm::WindowState* ShellSurfaceWidgetWrapper::GetWindowState() {
  return ash::wm::GetWindowState(widget_->GetNativeWindow());
}

gfx::Rect ShellSurfaceWidgetWrapper::GetBoundsForClientView() const {
  const views::NonClientView* non_client_view = widget_->non_client_view();
  return non_client_view->frame_view()->GetBoundsForClientView();
}

// static
void ShellSurfaceWidgetWrapper::SetClientControlledStateDelegateFactoryForTest(
    std::unique_ptr<ClientControlledStateDelegateFactory> factory) {
  g_delegate_factory = std::move(factory);
}

}  // namespace exo

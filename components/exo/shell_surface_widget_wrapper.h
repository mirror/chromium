// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_SHELL_SURFACE_WIDGET_WRAPPAR_H_
#define COMPONENTS_EXO_SHELL_SURFACE_WIDGET_WRAPPAR_H_

#include <memory>

#include "ash/public/interfaces/window_pin_type.mojom.h"
#include "ash/wm/client_controlled_state.h"
#include "base/macros.h"
#include "components/exo/shell_surface.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace wm {
class WindowState;
}
}  // namespace ash

namespace gfx {
class Rect;
}

namespace exo {

class ShellSurfaceWidgetWrapper {
 public:
  class ClientControlledStateDelegateFactory {
   public:
    virtual ~ClientControlledStateDelegateFactory() = default;
    virtual std::unique_ptr<ash::wm::ClientControlledState::Delegate>
    Create() = 0;
  };

  static void SetClientControlledStateDelegateFactory(
      std::unique_ptr<ClientControlledStateDelegateFactory> factory);

  static std::unique_ptr<ShellSurfaceWidgetWrapper> Create(
      ShellSurface* shell_surface);
  ShellSurfaceWidgetWrapper(ShellSurface* shell_surface, views::Widget* widget);
  virtual ~ShellSurfaceWidgetWrapper();

  virtual void Init();

  virtual void SyncInitialState();

  virtual void Restore();
  virtual void Minimize();
  virtual void Maximize();
  virtual void SetFullscreen(bool fullscreen);

  virtual void SetBounds(const gfx::Rect& bounds);
  virtual void SetBoundsInParent(const gfx::Rect& bounds);
  virtual void SetPinned(ash::mojom::WindowPinType type);

  gfx::NativeWindow GetNativeWindow();

  gfx::Rect GetBoundsForClientView() const;
  ash::wm::WindowState* GetWindowState();

  views::Widget* widget() { return widget_; }

 protected:
  ShellSurface* shell_surface_;

 private:
  views::Widget* const widget_;
  DISALLOW_COPY_AND_ASSIGN(ShellSurfaceWidgetWrapper);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_SHELL_SURFACE_WIDGET_WRAPPAR_H_

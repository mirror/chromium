// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_MUS_NATIVE_WIDGET_VIEW_MANAGER_H_
#define UI_VIEWS_MUS_NATIVE_WIDGET_VIEW_MANAGER_H_

#include "ui/views/widget/native_widget_aura.h"

namespace aura {
namespace client {
class DefaultCaptureClient;
}
}

namespace mojo {
class Shell;
}

namespace mus {
class Window;
namespace mojom {
class WindowManager;
}
}

namespace ui {
namespace internal {
class InputMethodDelegate;
}
}

namespace wm {
class FocusController;
}

namespace views {

namespace {
class NativeWidgetWindowObserver;
}

class WindowTreeHostMojo;

class NativeWidgetViewManager : public views::NativeWidgetAura {
 public:
  NativeWidgetViewManager(views::internal::NativeWidgetDelegate* delegate,
                          mojo::Shell* shell,
                          mus::Window* window);
  ~NativeWidgetViewManager() override;

  // TODO(beng): move to ctor.
  void set_window_manager(mus::mojom::WindowManager* window_manager) {
    window_manager_ = window_manager;
  }

 private:
  friend class NativeWidgetWindowObserver;

  // Overridden from internal::NativeWidgetAura:
  void InitNativeWidget(const views::Widget::InitParams& in_params) override;
  void OnWindowVisibilityChanged(aura::Window* window, bool visible) override;
  void CenterWindow(const gfx::Size& size) override;
  void SetBounds(const gfx::Rect& bounds) override;
  void SetSize(const gfx::Size& size) override;

  scoped_ptr<WindowTreeHostMojo> window_tree_host_;
  scoped_ptr<NativeWidgetWindowObserver> window_observer_;

  scoped_ptr<wm::FocusController> focus_client_;

  mus::Window* window_;

  scoped_ptr<aura::client::DefaultCaptureClient> capture_client_;

  mus::mojom::WindowManager* window_manager_;

  DISALLOW_COPY_AND_ASSIGN(NativeWidgetViewManager);
};

}  // namespace views

#endif  // UI_VIEWS_MUS_NATIVE_WIDGET_VIEW_MANAGER_H_

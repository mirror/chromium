// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/mus/native_widget_view_manager.h"

#include "components/mus/public/cpp/window.h"
#include "components/mus/public/interfaces/window_manager.mojom.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "ui/aura/client/default_capture_client.h"
#include "ui/aura/layout_manager.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/ime/input_method_delegate.h"
#include "ui/views/mus/input_method_mus.h"
#include "ui/views/mus/window_tree_host_mus.h"
#include "ui/wm/core/base_focus_rules.h"
#include "ui/wm/core/capture_controller.h"
#include "ui/wm/core/focus_controller.h"

namespace views {
namespace {

// TODO: figure out what this should be.
class FocusRulesImpl : public wm::BaseFocusRules {
 public:
  FocusRulesImpl() {}
  ~FocusRulesImpl() override {}

  bool SupportsChildActivation(aura::Window* window) const override {
    return true;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FocusRulesImpl);
};

class NativeWidgetWindowObserver : public mus::WindowObserver {
 public:
  NativeWidgetWindowObserver(NativeWidgetViewManager* view_manager)
      : view_manager_(view_manager) {
    view_manager_->window_->AddObserver(this);
  }

  ~NativeWidgetWindowObserver() override {
    if (view_manager_->window_)
      view_manager_->window_->RemoveObserver(this);
  }

 private:
  // WindowObserver:
  void OnWindowDestroyed(mus::Window* view) override {
    DCHECK_EQ(view, view_manager_->window_);
    view->RemoveObserver(this);
    view_manager_->window_ = nullptr;
    // TODO(sky): WindowTreeHostMus assumes the View outlives it.
    // NativeWidgetWindowObserver needs to deal, likely by deleting this.
  }

  NativeWidgetViewManager* const view_manager_;

  DISALLOW_COPY_AND_ASSIGN(NativeWidgetWindowObserver);
};

class WindowTreeHostWindowLayoutManager : public aura::LayoutManager {
 public:
  WindowTreeHostWindowLayoutManager(aura::Window* outer, aura::Window* inner)
      : outer_(outer), inner_(inner) {}
  ~WindowTreeHostWindowLayoutManager() override {}

 private:
  // aura::LayoutManager:
  void OnWindowResized() override { inner_->SetBounds(outer_->bounds()); }
  void OnWindowAddedToLayout(aura::Window* child) override {}
  void OnWillRemoveWindowFromLayout(aura::Window* child) override {}
  void OnWindowRemovedFromLayout(aura::Window* child) override {}
  void OnChildWindowVisibilityChanged(aura::Window* child,
                                      bool visible) override {}
  void SetChildBounds(aura::Window* child,
                      const gfx::Rect& requested_bounds) override {
    SetChildBoundsDirect(child, requested_bounds);
  }

  aura::Window* outer_;
  aura::Window* inner_;

  DISALLOW_COPY_AND_ASSIGN(WindowTreeHostWindowLayoutManager);
};

}  // namespace

NativeWidgetViewManager::NativeWidgetViewManager(
    views::internal::NativeWidgetDelegate* delegate,
    mojo::Shell* shell,
    mus::Window* window)
    : NativeWidgetAura(delegate), window_(window) {
  window_tree_host_.reset(
      new WindowTreeHostMus(shell, window_, mus::mojom::SURFACE_TYPE_DEFAULT));
  window_tree_host_->InitHost();

  focus_client_.reset(new wm::FocusController(new FocusRulesImpl));

  aura::client::SetFocusClient(window_tree_host_->window(),
                               focus_client_.get());
  aura::client::SetActivationClient(window_tree_host_->window(),
                                    focus_client_.get());
  window_tree_host_->window()->AddPreTargetHandler(focus_client_.get());
  window_tree_host_->window()->SetLayoutManager(
      new WindowTreeHostWindowLayoutManager(window_tree_host_->window(),
                                            GetNativeWindow()));

  capture_client_.reset(
      new aura::client::DefaultCaptureClient(window_tree_host_->window()));

  window_observer_.reset(new NativeWidgetWindowObserver(this));
}

NativeWidgetViewManager::~NativeWidgetViewManager() {}

void NativeWidgetViewManager::InitNativeWidget(
    const views::Widget::InitParams& in_params) {
  views::Widget::InitParams params(in_params);
  params.parent = window_tree_host_->window();
  NativeWidgetAura::InitNativeWidget(params);
}

void NativeWidgetViewManager::OnWindowVisibilityChanged(aura::Window* window,
                                                        bool visible) {
  window_->SetVisible(visible);
  // NOTE: We could also update aura::Window's visibility when the View's
  // visibility changes, but this code isn't going to be around for very long so
  // I'm not bothering.
}

}  // namespace views

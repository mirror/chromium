// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_NATIVE_NATIVE_VIEW_HOST_AURA_H_
#define UI_VIEWS_CONTROLS_NATIVE_NATIVE_VIEW_HOST_AURA_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_delegate.h"
#include "ui/views/controls/native/native_view_host_wrapper.h"
#include "ui/views/view_observer.h"
#include "ui/views/views_export.h"

namespace views {

class NativeViewHost;

namespace internal {

// This thunk is necessary because LayerDelegate and aura::WindowObserver both
// have an OnDelegatedFrameDamage method, meaning NativeViewHostAura can't
// inherit from both.
class LayerDelegateWrapper : public ui::LayerDelegate {
 public:
  void OnDelegatedFrameDamage(const gfx::Rect& damage_rect_in_dip) override;
  void OnDeviceScaleFactorChanged(float device_scale_factor) override;

 protected:
  LayerDelegateWrapper() {}
  ~LayerDelegateWrapper() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(LayerDelegateWrapper);
};

}  // namespace internal

// Aura implementation of NativeViewHostWrapper.
class VIEWS_EXPORT NativeViewHostAura : public NativeViewHostWrapper,
                                        public aura::WindowObserver,
                                        public internal::LayerDelegateWrapper {
 public:
  explicit NativeViewHostAura(NativeViewHost* host);
  ~NativeViewHostAura() override;

  // Overridden from NativeViewHostWrapper:
  void AttachNativeView() override;
  void NativeViewDetaching(bool destroyed) override;
  void AddedToWidget() override;
  void RemovedFromWidget() override;
  void SetMask(std::unique_ptr<views::Painter> painter) override;
  void InstallClip(int x, int y, int w, int h) override;
  bool HasInstalledClip() override;
  void UninstallClip() override;
  void ShowWidget(int x, int y, int w, int h) override;
  void HideWidget() override;
  void SetFocus() override;
  gfx::NativeViewAccessible GetNativeViewAccessible() override;
  gfx::NativeCursor GetCursor(int x, int y) override;

  // internal::LayerDelegateWrapper
  void OnPaintLayer(const ui::PaintContext& context) override;

 private:
  friend class NativeViewHostAuraTest;
  class ClippingWindowDelegate;

  // Overridden from aura::WindowObserver:
  void OnWindowDestroying(aura::Window* window) override;
  void OnWindowDestroyed(aura::Window* window) override;
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds) override;

  // Reparents the native view with the clipping window existing between it and
  // its old parent, so that the fast resize path works.
  void AddClippingWindow();

  // If the native view has been reparented via AddClippingWindow, this call
  // undoes it.
  void RemoveClippingWindow();

  // Sets or updates the mask layer on the native view's layer.
  void InstallMask();

  // Our associated NativeViewHost.
  NativeViewHost* host_;

  std::unique_ptr<ClippingWindowDelegate> clipping_window_delegate_;

  // Window that exists between the native view and the parent that allows for
  // clipping to occur. This is positioned in the coordinate space of
  // host_->GetWidget().
  aura::Window clipping_window_;
  std::unique_ptr<gfx::Rect> clip_rect_;

  std::unique_ptr<ui::Layer> mask_layer_;
  std::unique_ptr<views::Painter> mask_painter_;

  DISALLOW_COPY_AND_ASSIGN(NativeViewHostAura);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_NATIVE_NATIVE_VIEW_HOST_AURA_H_

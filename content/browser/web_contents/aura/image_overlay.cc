// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_contents/aura/image_overlay.h"

#include <utility>

#include "base/logging.h"
#include "ui/aura/window.h"
#include "ui/aura_extra/image_window_delegate.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_animator.h"
#include "ui/compositor/scoped_layer_animation_settings.h"

namespace content {

ImageOverlay::ImageOverlay(gfx::Image image) {
  aura_extra::ImageWindowDelegate* delegate =
      new aura_extra::ImageWindowDelegate();
  delegate->SetImage(std::move(image));
  window_ = std::make_unique<aura::Window>(delegate);
  window_->set_owned_by_parent(false);
  window_->SetName("ImageOverlay");
  window_->Init(ui::LAYER_TEXTURED);
  window_->SetTransparent(true);
  window_->AddObserver(this);
  window_->Show();

  LOG(ERROR) << "Showing image overlay " << this;
}

ImageOverlay::~ImageOverlay() {
  LOG(ERROR) << "Destroying image overlay";
}

void ImageOverlay::AnimateOut() {
  if (!window_ || window_->layer()->GetTargetOpacity() == 0)
    return;

  LOG(ERROR) << "Animating out image overlay";

  ui::Layer* layer = window_->layer();
  ui::LayerAnimator* animator = layer->GetAnimator();
  ui::ScopedLayerAnimationSettings settings(animator);
  settings.SetTransitionDuration(base::TimeDelta::FromSeconds(1));
  animator->AddObserver(this);
  layer->SetOpacity(0);
}

void ImageOverlay::ResizeWindow() {
  DCHECK(window_);
  DCHECK(window_->parent());
  gfx::Rect parent_bounds = window_->parent()->GetTargetBounds();
  window_->SetBounds(
      gfx::Rect(0, 0, parent_bounds.width(), parent_bounds.height()));
}

void ImageOverlay::OnWillRemoveWindow(aura::Window* window) {
  if (window_ && window_->parent() == window)
    window->RemoveObserver(this);
}

void ImageOverlay::OnWindowParentChanged(aura::Window* window,
                                         aura::Window* parent) {
  if (window_.get() == window && parent) {
    DCHECK_EQ(window->parent(), parent);
    parent->AddObserver(this);
    ResizeWindow();
  }
}

void ImageOverlay::OnWindowBoundsChanged(aura::Window* window,
                                         const gfx::Rect& old_bounds,
                                         const gfx::Rect& new_bounds) {
  if (window_ && window_->parent() == window)
    ResizeWindow();
}

void ImageOverlay::OnLayerAnimationEnded(ui::LayerAnimationSequence* sequence) {
  // Only AnimateOut() should animate the window.
  DCHECK_EQ(window_->layer()->opacity(), 0.0f);

  window_.reset();
}

void ImageOverlay::OnLayerAnimationAborted(
    ui::LayerAnimationSequence* sequence) {
  NOTREACHED();
}

void ImageOverlay::OnLayerAnimationScheduled(
    ui::LayerAnimationSequence* sequence) {}

}  // namespace content

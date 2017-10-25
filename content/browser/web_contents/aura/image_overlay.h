// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_AURA_IMAGE_OVERLAY_H_
#define CONTENT_BROWSER_WEB_CONTENTS_AURA_IMAGE_OVERLAY_H_

#include <memory>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "ui/aura/window_observer.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/gfx/image/image.h"

namespace aura {
class Window;
}  // namespace aura

namespace content {

// Manages a window on which an image is painted and which always has the same
// size as its parent.
//
// The window is created in the constructor. It is then destroyed in the
// destructor or when the animation started by AnimateOut() completes, whichever
// comes first. To position the window in a window hierarchy, use the pointer
// returned by window().
class CONTENT_EXPORT ImageOverlay : public aura::WindowObserver,
                                    public ui::LayerAnimationObserver {
 public:
  // |image| is the image painted on the window.
  ImageOverlay(gfx::Image image);
  ~ImageOverlay() override;

  // Animates the window out. Destroys the window when the animation completes.
  void AnimateOut();

  // Returns the window managed by this object. Is nullptr once the AnimateOut()
  // animation is complete.
  aura::Window* window() const { return window_.get(); }

 private:
  void ResizeWindow();

  // aura::WindowObserver:
  void OnWillRemoveWindow(aura::Window* window) override;
  void OnWindowParentChanged(aura::Window* window,
                             aura::Window* parent) override;
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds) override;

  // ui::LayerAnimationObserver:
  void OnLayerAnimationEnded(ui::LayerAnimationSequence* sequence) override;
  void OnLayerAnimationAborted(ui::LayerAnimationSequence* sequence) override;
  void OnLayerAnimationScheduled(ui::LayerAnimationSequence* sequence) override;

  std::unique_ptr<aura::Window> window_;

  DISALLOW_COPY_AND_ASSIGN(ImageOverlay);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_CONTENTS_AURA_IMAGE_OVERLAY_H_

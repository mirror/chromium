// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/overlay/overlay_surface_embedder.h"

#include "components/viz/common/surfaces/stub_surface_reference_factory.h"
#include "ui/compositor/layer.h"

OverlaySurfaceEmbedder::OverlaySurfaceEmbedder(OverlayWindow* window)
    : window_(window) {
  DCHECK(window_);

  layer_ = std::make_unique<ui::Layer>(ui::LAYER_TEXTURED);
  layer_->SetMasksToBounds(true);

  // The frame provided by the parent window's layer needs to show through
  // the surface layer.
  layer_->SetFillsBoundsOpaquely(false);
  layer_->SetBounds(gfx::Rect(gfx::Point(0, 0), window_->GetBounds().size()));

  // adds layer to initial widget layer
  window_->GetLayer()->Add(layer_.get());
  window_->GetLayer()->StackAtTop(layer_.get());
  ref_factory_ = new viz::StubSurfaceReferenceFactory();
}

OverlaySurfaceEmbedder::~OverlaySurfaceEmbedder() = default;

void OverlaySurfaceEmbedder::SetPrimarySurfaceId(
    const viz::SurfaceId& surface_id) {
  // SurfaceInfo has information about the embedded surface.
  // This call creates a new SurfaceLayer so you don't have to do it yourself.
  layer_->SetShowPrimarySurface(surface_id, window_->GetBounds().size(),
                                ref_factory_);
}

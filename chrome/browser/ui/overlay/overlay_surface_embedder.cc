// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/pip/pip_surface_embedder.h"

namespace {

// TODO(mfomitchev, samans): Remove these stub classes once the SurfaceReference
// work is complete.
class StubSurfaceReferenceFactory : public cc::SurfaceReferenceFactory {
 public:
   StubSurfaceReferenceFactory() = default;

   // cc::SurfaceReferenceFactory:
   base::Closure CreateReference(
     cc::SurfaceReferenceOwner* owner,
     const cc::SurfaceId& surface_id) const override {
       return base::Closure();
     }

  protected:
   ~StubSurfaceReferenceFactory() override = default;

   DISALLOW_COPY_AND_ASSIGN(StubSurfaceReferenceFactory);
 };
 }  // namespace

 PipSurfaceEmbedder::PipSurfaceEmbedder(PipWindow* window) : window_(window) {
         LOG(ERROR) << "PipSurfaceEmbedder::PipSurfaceEmbedder";
   surface_layer_ = base::MakeUnique<ui::Layer>(ui::LAYER_TEXTURED);
   surface_layer_->SetMasksToBounds(true);
   // The frame provided by the parent window->layer() needs to show through
   // the surface layer.
   surface_layer_->SetFillsBoundsOpaquely(false);
 
   window_->GetLayer()->Add(surface_layer_.get());
 
   ref_factory_ = new StubSurfaceReferenceFactory();
}

PipSurfaceEmbedder::~PipSurfaceEmbedder() = default;

void PipSurfaceEmbedder::SetPrimarySurfaceInfo(const cc::SurfaceInfo& surface_info) {\
        LOG(ERROR) << "PipSurfaceEmbedder::SetPrimarySurfaceInfo";
  surface_layer_->SetShowPrimarySurface(surface_info, ref_factory_);
  surface_layer_->SetBounds(gfx::Rect(window_->GetBounds().size()));
}

void PipSurfaceEmbedder::SetFallbackSurfaceInfo(const cc::SurfaceInfo& surface_info) {
        LOG(ERROR) << "PipSurfaceEmbedder::SetFallbackSurfaceInfo";
  surface_layer_->SetFallbackSurface(surface_info);
  // update size and gutters - ClientSurfaceEmbedder
}
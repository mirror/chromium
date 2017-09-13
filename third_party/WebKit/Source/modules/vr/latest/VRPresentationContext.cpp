// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRPresentationContext.h"

#include "bindings/modules/v8/rendering_context.h"
#include "core/imagebitmap/ImageBitmap.h"
#include "platform/graphics/GraphicsContext.h"
#include "platform/graphics/StaticBitmapImage.h"
#include "platform/graphics/gpu/ImageLayerBridge.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace blink {

VRPresentationContext::VRPresentationContext(
    CanvasRenderingContextHost* host,
    const CanvasContextCreationAttributes& attrs)
    : CanvasRenderingContext(host, attrs),
      image_layer_bridge_(
          new ImageLayerBridge(attrs.alpha() ? kNonOpaque : kOpaque)) {}

VRPresentationContext::~VRPresentationContext() {}

void VRPresentationContext::SetCanvasGetContextResult(
    RenderingContext& result) {
  result.setVRPresentationContext(this);
}

void VRPresentationContext::transferFromImageBitmap(ImageBitmap* image_bitmap) {
  /*if (image_bitmap && image_bitmap->IsNeutered()) {
    exception_state.ThrowDOMException(
        kInvalidStateError, "The input ImageBitmap has been detached");
    return;
  }*/

  image_layer_bridge_->SetImage(image_bitmap ? image_bitmap->BitmapImage()
                                             : nullptr);

  DidDraw();

  if (image_bitmap)
    image_bitmap->close();
}

CanvasRenderingContext* VRPresentationContext::Factory::Create(
    CanvasRenderingContextHost* host,
    const CanvasContextCreationAttributes& attrs) {
  if (!RuntimeEnabledFeatures::WebVREnabled() /*||
      OriginTrials::webVREnabled(document.GetExecutionContext())*/)
    return nullptr;
  return new VRPresentationContext(host, attrs);
}

void VRPresentationContext::Stop() {
  image_layer_bridge_->Dispose();
}

RefPtr<StaticBitmapImage> VRPresentationContext::GetImage(
    AccelerationHint,
    SnapshotReason) const {
  return image_layer_bridge_->GetImage();
}

WebLayer* VRPresentationContext::PlatformLayer() const {
  return image_layer_bridge_->PlatformLayer();
}

bool VRPresentationContext::IsPaintable() const {
  return !!image_layer_bridge_->GetImage();
}

DEFINE_TRACE(VRPresentationContext) {
  visitor->Trace(image_layer_bridge_);
  CanvasRenderingContext::Trace(visitor);
}

bool VRPresentationContext::IsAccelerated() const {
  return image_layer_bridge_->IsAccelerated();
}

}  // namespace blink

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XRPresentationContext_h
#define XRPresentationContext_h

#include "base/memory/scoped_refptr.h"
#include "core/html/canvas/CanvasRenderingContext.h"
#include "core/html/canvas/CanvasRenderingContextFactory.h"
#include "modules/ModulesExport.h"

namespace blink {

class ImageBitmap;
class ImageLayerBridge;

class MODULES_EXPORT XRPresentationContext final
    : public CanvasRenderingContext {
  DEFINE_WRAPPERTYPEINFO();

 public:
  class Factory : public CanvasRenderingContextFactory {
    WTF_MAKE_NONCOPYABLE(Factory);

   public:
    Factory() {}
    ~Factory() override {}

    CanvasRenderingContext* Create(
        CanvasRenderingContextHost*,
        const CanvasContextCreationAttributes&) override;
    CanvasRenderingContext::ContextType GetContextType() const override {
      return CanvasRenderingContext::kContextXRPresent;
    }
  };

  void transferFromImageBitmap(ImageBitmap*);

  // Script API
  HTMLCanvasElement* canvas() {
    DCHECK(!Host() || !Host()->IsOffscreenCanvas());
    return static_cast<HTMLCanvasElement*>(Host());
  }

  // CanvasRenderingContext implementation
  ContextType GetContextType() const override {
    return CanvasRenderingContext::kContextXRPresent;
  }
  void SetIsHidden(bool) override {}
  bool isContextLost() const override { return false; }
  void SetCanvasGetContextResult(RenderingContext&) final;
  scoped_refptr<StaticBitmapImage> GetImage(AccelerationHint,
                                            SnapshotReason) const final;
  bool IsComposited() const final { return true; }
  bool IsAccelerated() const final;

  WebLayer* PlatformLayer() const final;
  // TODO(junov): handle lost contexts when content is GPU-backed
  void LoseContext(LostContextMode) override {}

  void Stop() override;

  bool IsPaintable() const final;

  virtual ~XRPresentationContext();

  void Trace(blink::Visitor*) override;

 private:
  XRPresentationContext(CanvasRenderingContextHost*,
                        const CanvasContextCreationAttributes&);

  Member<ImageLayerBridge> image_layer_bridge_;
};

DEFINE_TYPE_CASTS(XRPresentationContext,
                  CanvasRenderingContext,
                  context,
                  context->GetContextType() ==
                      CanvasRenderingContext::kContextXRPresent,
                  context.GetContextType() ==
                      CanvasRenderingContext::kContextXRPresent);

}  // namespace blink

#endif  // XRPresentationContext_h

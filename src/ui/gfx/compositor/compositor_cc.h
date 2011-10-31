// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_COMPOSITOR_COMPOSITOR_CC_H_
#define UI_GFX_COMPOSITOR_COMPOSITOR_CC_H_
#pragma once

#include "base/compiler_specific.h"
#include "base/memory/singleton.h"
#include "base/memory/ref_counted.h"
#include "ui/gfx/compositor/compositor.h"
#include "ui/gfx/size.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebLayer.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebLayerClient.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebLayerTreeView.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebLayerTreeViewClient.h"

namespace gfx {
class Rect;
class GLContext;
class GLSurface;
class GLShareGroup;
}

namespace ui {

class COMPOSITOR_EXPORT SharedResourcesCC : public SharedResources {
 public:
  static SharedResourcesCC* GetInstance();

  virtual bool MakeSharedContextCurrent();
  virtual void* GetDisplay();

  gfx::GLShareGroup* GetShareGroup();

 private:
  friend struct DefaultSingletonTraits<SharedResourcesCC>;

  SharedResourcesCC();
  virtual ~SharedResourcesCC();

  bool Initialize();
  void Destroy();

  bool initialized_;

  scoped_refptr<gfx::GLContext> context_;
  scoped_refptr<gfx::GLSurface> surface_;

  DISALLOW_COPY_AND_ASSIGN(SharedResourcesCC);
};

class COMPOSITOR_EXPORT TextureCC : public Texture {
 public:
  TextureCC();

  // Texture implementation.
  virtual void SetCanvas(const SkCanvas& canvas,
                         const gfx::Point& origin,
                         const gfx::Size& overall_size) OVERRIDE;

  virtual void Draw(const ui::TextureDrawParams& params,
                    const gfx::Rect& clip_bounds_in_texture) OVERRIDE;

  virtual void Update() = 0;
  unsigned int texture_id() const { return texture_id_; }
  bool flipped() const { return flipped_; }

 protected:
  unsigned int texture_id_;
  bool flipped_;
  DISALLOW_COPY_AND_ASSIGN(TextureCC);
};

class COMPOSITOR_EXPORT CompositorCC : public Compositor,
                                       public WebKit::WebLayerTreeViewClient,
                                       public WebKit::WebLayerClient {
 public:
  CompositorCC(CompositorDelegate* delegate,
               gfx::AcceleratedWidget widget,
               const gfx::Size& size);
  virtual ~CompositorCC();

  static void InitializeThread();
  static void TerminateThread();

 protected:
  // Compositor implementation.
  virtual Texture* CreateTexture() OVERRIDE;
  virtual void Blur(const gfx::Rect& bounds) OVERRIDE;
  virtual void ScheduleDraw() OVERRIDE;
  virtual void OnNotifyStart(bool clear) OVERRIDE;
  virtual void OnNotifyEnd() OVERRIDE;
  virtual void OnWidgetSizeChanged() OVERRIDE;
  virtual void OnRootLayerChanged() OVERRIDE;
  virtual void DrawTree() OVERRIDE;

  // WebLayerTreeViewClient implementation.
  virtual void animateAndLayout(double frameBeginTime) OVERRIDE;
  virtual void applyScrollDelta(const WebKit::WebSize&) OVERRIDE;
  virtual WebKit::WebGraphicsContext3D* createContext3D() OVERRIDE;
  virtual void didRebindGraphicsContext(bool success) OVERRIDE;
  virtual void scheduleComposite() OVERRIDE;

  // WebLayerClient implementation.
  virtual void notifyNeedsComposite() OVERRIDE;

 private:
  gfx::AcceleratedWidget widget_;
  WebKit::WebLayer root_web_layer_;
  WebKit::WebLayerTreeView host_;

  DISALLOW_COPY_AND_ASSIGN(CompositorCC);
};

}  // namespace ui

#endif  // UI_GFX_COMPOSITOR_COMPOSITOR_CC_H_

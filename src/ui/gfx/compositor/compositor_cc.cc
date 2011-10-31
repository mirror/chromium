// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/compositor/compositor_cc.h"

#include "third_party/WebKit/Source/WebKit/chromium/public/WebCompositor.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFloatPoint.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebSize.h"
#include "ui/gfx/compositor/layer.h"
#include "ui/gfx/gl/gl_context.h"
#include "ui/gfx/gl/gl_surface.h"
#include "ui/gfx/gl/gl_implementation.h"
#include "webkit/glue/webthread_impl.h"
#include "webkit/gpu/webgraphicscontext3d_in_process_impl.h"

namespace {
webkit_glue::WebThreadImpl* g_compositor_thread = NULL;
}  // anonymous namespace

namespace ui {

SharedResourcesCC::SharedResourcesCC() : initialized_(false) {
}


SharedResourcesCC::~SharedResourcesCC() {
}

// static
SharedResourcesCC* SharedResourcesCC::GetInstance() {
  // We use LeakySingletonTraits so that we don't race with
  // the tear down of the gl_bindings.
  SharedResourcesCC* instance = Singleton<SharedResourcesCC,
      LeakySingletonTraits<SharedResourcesCC> >::get();
  if (instance->Initialize()) {
    return instance;
  } else {
    instance->Destroy();
    return NULL;
  }
}

bool SharedResourcesCC::Initialize() {
  if (initialized_)
    return true;

  {
    // The following line of code exists soley to disable IO restrictions
    // on this thread long enough to perform the GL bindings.
    // TODO(wjmaclean) Remove this when GL initialisation cleaned up.
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    if (!gfx::GLSurface::InitializeOneOff() ||
        gfx::GetGLImplementation() == gfx::kGLImplementationNone) {
      LOG(ERROR) << "Could not load the GL bindings";
      return false;
    }
  }

  surface_ = gfx::GLSurface::CreateOffscreenGLSurface(false, gfx::Size(1, 1));
  if (!surface_.get()) {
    LOG(ERROR) << "Unable to create offscreen GL surface.";
    return false;
  }

  context_ = gfx::GLContext::CreateGLContext(
      NULL, surface_.get(), gfx::PreferIntegratedGpu);
  if (!context_.get()) {
    LOG(ERROR) << "Unable to create GL context.";
    return false;
  }

  initialized_ = true;
  return true;
}

void SharedResourcesCC::Destroy() {
  context_ = NULL;
  surface_ = NULL;

  initialized_ = false;
}

bool SharedResourcesCC::MakeSharedContextCurrent() {
  DCHECK(initialized_);
  return context_->MakeCurrent(surface_.get());
}

void* SharedResourcesCC::GetDisplay() {
  return surface_->GetDisplay();
}

gfx::GLShareGroup* SharedResourcesCC::GetShareGroup() {
  DCHECK(initialized_);
  return context_->share_group();
}

TextureCC::TextureCC()
    : texture_id_(0),
      flipped_(false) {
}

void TextureCC::SetCanvas(const SkCanvas& canvas,
                          const gfx::Point& origin,
                          const gfx::Size& overall_size) {
  NOTREACHED();
}

void TextureCC::Draw(const ui::TextureDrawParams& params,
                     const gfx::Rect& clip_bounds_in_texture) {
  NOTREACHED();
}

CompositorCC::CompositorCC(CompositorDelegate* delegate,
                           gfx::AcceleratedWidget widget,
                           const gfx::Size& size)
    : Compositor(delegate, size),
      widget_(widget),
      root_web_layer_(WebKit::WebLayer::create(this)) {
  WebKit::WebLayerTreeView::Settings settings;
  settings.enableCompositorThread = !!g_compositor_thread;
  host_ = WebKit::WebLayerTreeView::create(this, root_web_layer_, settings);
  root_web_layer_.setAnchorPoint(WebKit::WebFloatPoint(0.f, 0.f));
  OnWidgetSizeChanged();
}

CompositorCC::~CompositorCC() {
}

void CompositorCC::InitializeThread() {
  g_compositor_thread = new webkit_glue::WebThreadImpl("Browser Compositor");
  WebKit::WebCompositor::setThread(g_compositor_thread);
}

void CompositorCC::TerminateThread() {
  DCHECK(g_compositor_thread);
  delete g_compositor_thread;
  g_compositor_thread = NULL;
}

Texture* CompositorCC::CreateTexture() {
  NOTREACHED();
  return NULL;
}

void CompositorCC::Blur(const gfx::Rect& bounds) {
  NOTIMPLEMENTED();
}

void CompositorCC::ScheduleDraw() {
  if (g_compositor_thread)
    host_.composite();
  else
    Compositor::ScheduleDraw();
}

void CompositorCC::OnNotifyStart(bool clear) {
}

void CompositorCC::OnNotifyEnd() {
}

void CompositorCC::OnWidgetSizeChanged() {
  host_.setViewportSize(size());
  root_web_layer_.setBounds(size());
}

void CompositorCC::OnRootLayerChanged() {
  root_web_layer_.removeAllChildren();
  root_web_layer_.addChild(root_layer()->web_layer());
}

void CompositorCC::DrawTree() {
  host_.composite();
}

void CompositorCC::animateAndLayout(double frameBeginTime) {
}

void CompositorCC::applyScrollDelta(const WebKit::WebSize&) {
}

WebKit::WebGraphicsContext3D* CompositorCC::createContext3D() {
  gfx::GLShareGroup* share_group =
      SharedResourcesCC::GetInstance()->GetShareGroup();
  WebKit::WebGraphicsContext3D* context =
      new webkit::gpu::WebGraphicsContext3DInProcessImpl(widget_, share_group);
  WebKit::WebGraphicsContext3D::Attributes attrs;
  context->initialize(attrs, 0, true);
  return context;
}

void CompositorCC::didRebindGraphicsContext(bool success) {
}

void CompositorCC::scheduleComposite() {
  ScheduleDraw();
}

void CompositorCC::notifyNeedsComposite() {
  ScheduleDraw();
}

Compositor* Compositor::Create(CompositorDelegate* owner,
                               gfx::AcceleratedWidget widget,
                               const gfx::Size& size) {
  return new CompositorCC(owner, widget, size);
}

}  // namespace ui

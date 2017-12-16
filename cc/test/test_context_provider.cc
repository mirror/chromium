// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/test_context_provider.h"

#include <stddef.h>
#include <stdint.h>

#include <set>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "cc/test/test_gles2_interface.h"
#include "cc/test/test_web_graphics_context_3d.h"
#include "components/viz/common/gpu/context_cache_controller.h"
#include "gpu/command_buffer/client/raster_implementation_gles.h"
#include "gpu/skia_bindings/grcontext_for_gles2_interface.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "third_party/skia/include/gpu/gl/GrGLInterface.h"

namespace cc {

namespace {

// Various tests rely on functionality (capabilities) enabled by these extension
// strings.
const char* const kExtensions[] = {"GL_EXT_stencil_wrap",
                                   "GL_EXT_texture_format_BGRA8888",
                                   "GL_OES_rgb8_rgba8",
                                   "GL_EXT_texture_norm16",
                                   "GL_CHROMIUM_framebuffer_multisample",
                                   "GL_CHROMIUM_renderbuffer_format_BGRA8888"};

class TestGLES2InterfaceForContextProvider : public TestGLES2Interface {
 public:
  TestGLES2InterfaceForContextProvider()
      : extension_string_(BuildExtensionString()) {}
  ~TestGLES2InterfaceForContextProvider() override = default;

  // TestGLES2Interface:
  const GLubyte* GetString(GLenum name) override {
    switch (name) {
      case GL_EXTENSIONS:
        return reinterpret_cast<const GLubyte*>(extension_string_.c_str());
      case GL_VERSION:
        return reinterpret_cast<const GrGLubyte*>("4.0 Null GL");
      case GL_SHADING_LANGUAGE_VERSION:
        return reinterpret_cast<const GrGLubyte*>("4.20.8 Null GLSL");
      case GL_VENDOR:
        return reinterpret_cast<const GrGLubyte*>("Null Vendor");
      case GL_RENDERER:
        return reinterpret_cast<const GrGLubyte*>("The Null (Non-)Renderer");
    }
    return nullptr;
  }
  const GrGLubyte* GetStringi(GrGLenum name, GrGLuint i) override {
    if (name == GL_EXTENSIONS && i < arraysize(kExtensions))
      return reinterpret_cast<const GLubyte*>(kExtensions[i]);
    return nullptr;
  }
  void GetIntegerv(GLenum name, GLint* params) override {
    switch (name) {
      case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
        *params = 8;
        return;
      case GL_MAX_TEXTURE_SIZE:
        *params = 2048;
        break;
      case GL_MAX_RENDERBUFFER_SIZE:
        *params = 2048;
        break;
      default:
        break;
    }
    TestGLES2Interface::GetIntegerv(name, params);
  }

 private:
  static std::string BuildExtensionString() {
    std::string extension_string = kExtensions[0];
    for (size_t i = 1; i < arraysize(kExtensions); ++i) {
      extension_string += " ";
      extension_string += kExtensions[i];
    }
    return extension_string;
  }

  const std::string extension_string_;

  DISALLOW_COPY_AND_ASSIGN(TestGLES2InterfaceForContextProvider);
};

}  // namespace

TestContextProviderBase::TestContextProviderBase(
    std::unique_ptr<TestContextSupport> support,
    std::unique_ptr<TestGLES2Interface> gl,
    std::unique_ptr<TestWebGraphicsContext3D> context,
    bool support_locking)
    : support_(std::move(support)),
      context3d_(std::move(context)),
      context_gl_(std::move(gl)),
      support_locking_(support_locking),
      weak_ptr_factory_(this) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  DCHECK(context3d_);
  DCHECK(context_gl_);
  context_thread_checker_.DetachFromThread();
  context_gl_->set_test_context(context3d_.get());
  context3d_->set_test_support(support_.get());
  raster_context_ = std::make_unique<gpu::raster::RasterImplementationGLES>(
      context_gl_.get(), context3d_->test_capabilities());
  // Just pass nullptr to the viz::ContextCacheController for its task runner.
  // Idle handling is tested directly in viz::ContextCacheController's
  // unittests, and isn't needed here.
  cache_controller_.reset(
      new viz::ContextCacheController(support_.get(), nullptr));
}

TestContextProviderBase::~TestContextProviderBase() {
  DCHECK(main_thread_checker_.CalledOnValidThread() ||
         context_thread_checker_.CalledOnValidThread());
}

gpu::ContextResult TestContextProviderBase::BindToCurrentThread() {
  // This is called on the thread the context will be used.
  DCHECK(context_thread_checker_.CalledOnValidThread());

  if (!bound_) {
    if (context_gl_->GetGraphicsResetStatusKHR() != GL_NO_ERROR)
      return gpu::ContextResult::kTransientFailure;

    context3d_->set_context_lost_callback(base::Bind(
        &TestContextProviderBase::OnLostContext, base::Unretained(this)));
  }
  bound_ = true;
  return gpu::ContextResult::kSuccess;
}

const gpu::Capabilities& TestContextProviderBase::ContextCapabilities() const {
  DCHECK(bound_);
  CheckValidThreadOrLockAcquired();
  return context3d_->test_capabilities();
}

const gpu::GpuFeatureInfo& TestContextProviderBase::GetGpuFeatureInfo() const {
  DCHECK(bound_);
  CheckValidThreadOrLockAcquired();
  return gpu_feature_info_;
}

gpu::gles2::GLES2Interface* TestContextProviderBase::ContextGL() {
  DCHECK(context3d_);
  DCHECK(bound_);
  CheckValidThreadOrLockAcquired();

  return context_gl_.get();
}

gpu::raster::RasterInterface* TestContextProviderBase::RasterInterface() {
  return raster_context_.get();
}

gpu::ContextSupport* TestContextProviderBase::ContextSupport() {
  return support();
}

class GrContext* TestContextProviderBase::GrContext() {
  DCHECK(bound_);
  CheckValidThreadOrLockAcquired();

  if (gr_context_)
    return gr_context_->get();

  size_t max_resource_cache_bytes;
  size_t max_glyph_cache_texture_bytes;
  skia_bindings::GrContextForGLES2Interface::DefaultCacheLimitsForTests(
      &max_resource_cache_bytes, &max_glyph_cache_texture_bytes);
  gr_context_ = std::make_unique<skia_bindings::GrContextForGLES2Interface>(
      context_gl_.get(), context3d_->test_capabilities(),
      max_resource_cache_bytes, max_glyph_cache_texture_bytes);
  cache_controller_->SetGrContext(gr_context_->get());

  // If GlContext is already lost, also abandon the new GrContext.
  if (ContextGL()->GetGraphicsResetStatusKHR() != GL_NO_ERROR)
    gr_context_->get()->abandonContext();

  return gr_context_->get();
}

viz::ContextCacheController* TestContextProviderBase::CacheController() {
  CheckValidThreadOrLockAcquired();
  return cache_controller_.get();
}

void TestContextProviderBase::InvalidateGrContext(uint32_t state) {
  DCHECK(bound_);
  CheckValidThreadOrLockAcquired();

  if (gr_context_)
    gr_context_->get()->resetContext(state);
}

base::Lock* TestContextProviderBase::GetLock() {
  return &context_lock_;
}

void TestContextProviderBase::OnLostContext() {
  CheckValidThreadOrLockAcquired();
  for (auto& observer : observers_)
    observer.OnContextLost();
  if (gr_context_)
    gr_context_->get()->abandonContext();
}

TestWebGraphicsContext3D* TestContextProviderBase::TestContext3d() {
  DCHECK(bound_);
  CheckValidThreadOrLockAcquired();

  return context3d_.get();
}

TestWebGraphicsContext3D* TestContextProviderBase::UnboundTestContext3d() {
  return context3d_.get();
}

void TestContextProviderBase::AddObserver(viz::ContextLostObserver* obs) {
  observers_.AddObserver(obs);
}

void TestContextProviderBase::RemoveObserver(viz::ContextLostObserver* obs) {
  observers_.RemoveObserver(obs);
}

TestContextProvider::TestContextProvider(
    std::unique_ptr<TestContextSupport> support,
    std::unique_ptr<TestGLES2Interface> gl,
    std::unique_ptr<TestWebGraphicsContext3D> context,
    bool support_locking)
    : TestContextProviderBase(std::move(support),
                              std::move(gl),
                              std::move(context),
                              support_locking) {}

gpu::ContextResult TestContextProvider::BindToCurrentThread() {
  return TestContextProviderBase::BindToCurrentThread();
}
gpu::ContextSupport* TestContextProvider::ContextSupport() {
  return TestContextProviderBase::ContextSupport();
}
class GrContext* TestContextProvider::GrContext() {
  return TestContextProviderBase::GrContext();
}
viz::ContextCacheController* TestContextProvider::CacheController() {
  return TestContextProviderBase::CacheController();
}
void TestContextProvider::InvalidateGrContext(uint32_t state) {
  return TestContextProviderBase::InvalidateGrContext(state);
}
base::Lock* TestContextProvider::GetLock() {
  return TestContextProviderBase::GetLock();
}
const gpu::Capabilities& TestContextProvider::ContextCapabilities() const {
  return TestContextProviderBase::ContextCapabilities();
}
const gpu::GpuFeatureInfo& TestContextProvider::GetGpuFeatureInfo() const {
  return TestContextProviderBase::GetGpuFeatureInfo();
}
void TestContextProvider::AddObserver(viz::ContextLostObserver* obs) {
  return TestContextProviderBase::AddObserver(obs);
}
void TestContextProvider::RemoveObserver(viz::ContextLostObserver* obs) {
  return TestContextProviderBase::RemoveObserver(obs);
}

gpu::gles2::GLES2Interface* TestContextProvider::ContextGL() {
  return TestContextProviderBase::ContextGL();
}

// static
scoped_refptr<TestContextProvider> TestContextProvider::Create() {
  constexpr bool support_locking = false;
  return new TestContextProvider(
      std::make_unique<TestContextSupport>(),
      std::make_unique<TestGLES2InterfaceForContextProvider>(),
      TestWebGraphicsContext3D::Create(), support_locking);
}

// static
scoped_refptr<TestContextProvider> TestContextProvider::Create(
    std::unique_ptr<TestWebGraphicsContext3D> context) {
  DCHECK(context);
  constexpr bool support_locking = false;
  return new TestContextProvider(
      std::make_unique<TestContextSupport>(),
      std::make_unique<TestGLES2InterfaceForContextProvider>(),
      std::move(context), support_locking);
}

// static
scoped_refptr<TestContextProvider> TestContextProvider::Create(
    std::unique_ptr<TestGLES2Interface> gl) {
  DCHECK(gl);
  constexpr bool support_locking = false;
  return new TestContextProvider(
      std::make_unique<TestContextSupport>(), std::move(gl),
      TestWebGraphicsContext3D::Create(), support_locking);
}

// static
scoped_refptr<TestContextProvider> TestContextProvider::Create(
    std::unique_ptr<TestWebGraphicsContext3D> context,
    std::unique_ptr<TestContextSupport> support) {
  DCHECK(context);
  DCHECK(support);
  constexpr bool support_locking = false;
  return new TestContextProvider(
      std::move(support),
      std::make_unique<TestGLES2InterfaceForContextProvider>(),
      std::move(context), support_locking);
}

TestRasterContextProvider::TestRasterContextProvider(
    std::unique_ptr<TestContextSupport> support,
    std::unique_ptr<TestGLES2Interface> gl,
    std::unique_ptr<TestWebGraphicsContext3D> context,
    bool support_locking)
    : TestContextProviderBase(std::move(support),
                              std::move(gl),
                              std::move(context),
                              support_locking) {}

gpu::ContextResult TestRasterContextProvider::BindToCurrentThread() {
  return TestContextProviderBase::BindToCurrentThread();
}
gpu::ContextSupport* TestRasterContextProvider::ContextSupport() {
  return TestContextProviderBase::ContextSupport();
}
class GrContext* TestRasterContextProvider::GrContext() {
  return TestContextProviderBase::GrContext();
}
viz::ContextCacheController* TestRasterContextProvider::CacheController() {
  return TestContextProviderBase::CacheController();
}
void TestRasterContextProvider::InvalidateGrContext(uint32_t state) {
  return TestContextProviderBase::InvalidateGrContext(state);
}
base::Lock* TestRasterContextProvider::GetLock() {
  return TestContextProviderBase::GetLock();
}
const gpu::Capabilities& TestRasterContextProvider::ContextCapabilities()
    const {
  return TestContextProviderBase::ContextCapabilities();
}
const gpu::GpuFeatureInfo& TestRasterContextProvider::GetGpuFeatureInfo()
    const {
  return TestContextProviderBase::GetGpuFeatureInfo();
}
void TestRasterContextProvider::AddObserver(viz::ContextLostObserver* obs) {
  return TestContextProviderBase::AddObserver(obs);
}
void TestRasterContextProvider::RemoveObserver(viz::ContextLostObserver* obs) {
  return TestContextProviderBase::RemoveObserver(obs);
}

gpu::raster::RasterInterface* TestRasterContextProvider::RasterInterface() {
  return TestContextProviderBase::RasterInterface();
}

// static
scoped_refptr<TestRasterContextProvider>
TestRasterContextProvider::CreateWorker() {
  constexpr bool support_locking = true;
  scoped_refptr<TestRasterContextProvider> worker_context_provider(
      new TestRasterContextProvider(
          std::make_unique<TestContextSupport>(),
          std::make_unique<TestGLES2InterfaceForContextProvider>(),
          TestWebGraphicsContext3D::Create(), support_locking));
  // Worker contexts are bound to the thread they are created on.
  auto result = worker_context_provider->BindToCurrentThread();
  if (result != gpu::ContextResult::kSuccess)
    return nullptr;
  return worker_context_provider;
}

// static
scoped_refptr<TestRasterContextProvider>
TestRasterContextProvider::CreateWorker(
    std::unique_ptr<TestWebGraphicsContext3D> context,
    std::unique_ptr<TestContextSupport> support) {
  DCHECK(context);
  DCHECK(support);
  constexpr bool support_locking = true;
  scoped_refptr<TestRasterContextProvider> worker_context_provider(
      new TestRasterContextProvider(
          std::move(support),
          std::make_unique<TestGLES2InterfaceForContextProvider>(),
          std::move(context), support_locking));
  // Worker contexts are bound to the thread they are created on.
  auto result = worker_context_provider->BindToCurrentThread();
  if (result != gpu::ContextResult::kSuccess)
    return nullptr;
  return worker_context_provider;
}

}  // namespace cc

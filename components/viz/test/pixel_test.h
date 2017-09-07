// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_util.h"
#include "cc/output/output_surface.h"
#include "cc/output/software_renderer.h"
#include "cc/quads/render_pass.h"
#include "cc/test/pixel_comparator.h"
#include "cc/trees/layer_tree_settings.h"
#include "components/viz/service/display/gl_renderer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gl/gl_implementation.h"

#ifndef COMPONENTS_VIZ_TEST_PIXEL_TEST_H_
#define COMPONENTS_VIZ_TEST_PIXEL_TEST_H_

namespace cc {
class DisplayResourceProvider;
class DirectRenderer;
class FakeOutputSurfaceClient;
class OutputSurface;
class SoftwareRenderer;
class TestSharedBitmapManager;
}  // namespace cc

namespace viz {
class CopyOutputResult;
class TestGpuMemoryBufferManager;

class PixelTest : public testing::Test {
 protected:
  PixelTest();
  ~PixelTest() override;

  bool RunPixelTest(cc::RenderPassList* pass_list,
                    const base::FilePath& ref_file,
                    const cc::PixelComparator& comparator);

  bool RunPixelTest(cc::RenderPassList* pass_list,
                    std::vector<SkColor>* ref_pixels,
                    const cc::PixelComparator& comparator);

  bool RunPixelTestWithReadbackTarget(cc::RenderPassList* pass_list,
                                      cc::RenderPass* target,
                                      const base::FilePath& ref_file,
                                      const cc::PixelComparator& comparator);

  bool RunPixelTestWithReadbackTargetAndArea(
      cc::RenderPassList* pass_list,
      cc::RenderPass* target,
      const base::FilePath& ref_file,
      const cc::PixelComparator& comparator,
      const gfx::Rect* copy_rect);

  ContextProvider* context_provider() const {
    return output_surface_->context_provider();
  }

  cc::LayerTreeSettings settings_;
  RendererSettings renderer_settings_;
  gfx::Size device_viewport_size_;
  bool disable_picture_quad_image_filtering_;
  std::unique_ptr<cc::FakeOutputSurfaceClient> output_surface_client_;
  std::unique_ptr<cc::OutputSurface> output_surface_;
  std::unique_ptr<cc::TestSharedBitmapManager> shared_bitmap_manager_;
  std::unique_ptr<TestGpuMemoryBufferManager> gpu_memory_buffer_manager_;
  std::unique_ptr<cc::BlockingTaskRunner> main_thread_task_runner_;
  std::unique_ptr<cc::DisplayResourceProvider> resource_provider_;
  std::unique_ptr<cc::TextureMailboxDeleter> texture_mailbox_deleter_;
  std::unique_ptr<cc::DirectRenderer> renderer_;
  cc::SoftwareRenderer* software_renderer_ = nullptr;
  std::unique_ptr<SkBitmap> result_bitmap_;

  void SetUpGLRenderer(bool flipped_output_surface);
  void SetUpSoftwareRenderer();

  void EnableExternalStencilTest();

 private:
  void ReadbackResult(base::Closure quit_run_loop,
                      std::unique_ptr<CopyOutputResult> result);

  bool PixelsMatchReference(const base::FilePath& ref_file,
                            const cc::PixelComparator& comparator);

  std::unique_ptr<gl::DisableNullDrawGLBindings> enable_pixel_output_;
};

template <typename RendererType>
class RendererPixelTest : public PixelTest {
 public:
  RendererType* renderer() {
    return static_cast<RendererType*>(renderer_.get());
  }

 protected:
  void SetUp() override;
};

// Wrappers to differentiate renderers where the the output surface and viewport
// have an externally determined size and offset.
class GLRendererWithExpandedViewport : public GLRenderer {
 public:
  GLRendererWithExpandedViewport(
      const RendererSettings* settings,
      cc::OutputSurface* output_surface,
      cc::DisplayResourceProvider* resource_provider,
      cc::TextureMailboxDeleter* texture_mailbox_deleter)
      : GLRenderer(settings,
                   output_surface,
                   resource_provider,
                   texture_mailbox_deleter) {}
};

class SoftwareRendererWithExpandedViewport : public cc::SoftwareRenderer {
 public:
  SoftwareRendererWithExpandedViewport(
      const RendererSettings* settings,
      cc::OutputSurface* output_surface,
      cc::DisplayResourceProvider* resource_provider)
      : SoftwareRenderer(settings, output_surface, resource_provider) {}
};

class GLRendererWithFlippedSurface : public GLRenderer {
 public:
  GLRendererWithFlippedSurface(
      const RendererSettings* settings,
      cc::OutputSurface* output_surface,
      cc::DisplayResourceProvider* resource_provider,
      cc::TextureMailboxDeleter* texture_mailbox_deleter)
      : GLRenderer(settings,
                   output_surface,
                   resource_provider,
                   texture_mailbox_deleter) {}
};

template <>
inline void RendererPixelTest<GLRenderer>::SetUp() {
  SetUpGLRenderer(false);
}

template <>
inline void RendererPixelTest<GLRendererWithExpandedViewport>::SetUp() {
  SetUpGLRenderer(false);
}

template <>
inline void RendererPixelTest<GLRendererWithFlippedSurface>::SetUp() {
  SetUpGLRenderer(true);
}

template <>
inline void RendererPixelTest<cc::SoftwareRenderer>::SetUp() {
  SetUpSoftwareRenderer();
}

template <>
inline void RendererPixelTest<SoftwareRendererWithExpandedViewport>::SetUp() {
  SetUpSoftwareRenderer();
}

typedef RendererPixelTest<GLRenderer> GLRendererPixelTest;
typedef RendererPixelTest<cc::SoftwareRenderer> SoftwareRendererPixelTest;

}  // namespace viz

#endif  // COMPONENTS_VIZ_TEST_PIXEL_TEST_H_

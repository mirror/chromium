// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/hash.h"
#include "build/build_config.h"
#include "cc/layers/picture_image_layer.h"
#include "cc/layers/solid_color_layer.h"
#include "cc/paint/paint_image_builder.h"
#include "cc/test/layer_tree_pixel_resource_test.h"
#include "cc/test/pixel_comparator.h"
#include "components/viz/test/test_layer_tree_frame_sink.h"

#if !defined(OS_ANDROID)

namespace cc {
namespace {

constexpr SkColor kCSSBlack = SkColorSetRGB(0, 0, 0);

scoped_refptr<Layer> CreateCheckerboardLayer(const gfx::Size& bounds) {
  constexpr int kGridSize = 8;
  constexpr SkColor color_even = SkColorSetRGB(153, 153, 153);
  constexpr SkColor color_odd = SkColorSetRGB(102, 102, 102);

  sk_sp<SkSurface> backing_store =
      SkSurface::MakeRasterN32Premul(bounds.width(), bounds.height());
  SkCanvas* canvas = backing_store->getCanvas();
  canvas->clear(color_even);

  SkPaint paint;
  paint.setColor(color_odd);
  for (int j = 0; j < (bounds.height() + kGridSize - 1) / kGridSize; j++) {
    for (int i = 0; i < (bounds.width() + kGridSize - 1) / kGridSize; i++) {
      if ((i ^ j ^ 1) & 1)
        continue;
      canvas->drawRect(
          SkRect::MakeXYWH(i * kGridSize, j * kGridSize, kGridSize, kGridSize),
          paint);
    }
  }
  scoped_refptr<PictureImageLayer> layer = PictureImageLayer::Create();
  layer->SetIsDrawable(true);
  layer->SetBounds(bounds);
  layer->SetImage(PaintImageBuilder()
                      .set_id(PaintImage::GetNextId())
                      .set_image(backing_store->makeImageSnapshot())
                      .TakePaintImage());
  return layer;
}

scoped_refptr<Layer> CreateRandomColorfulLayer(const gfx::Size& bounds,
                                               int grid_size,
                                               unsigned salt = 0xfeedbeef) {
  sk_sp<SkSurface> backing_store =
      SkSurface::MakeRasterN32Premul(bounds.width(), bounds.height());
  SkCanvas* canvas = backing_store->getCanvas();
  for (int j = 0; j < (bounds.height() + grid_size - 1) / grid_size; j++) {
    for (int i = 0; i < (bounds.width() + grid_size - 1) / grid_size; i++) {
      unsigned seed = salt ^ i ^ j << 16;
      uint32_t random_number = base::PersistentHash(&seed, sizeof(unsigned));
      int a = random_number % 6 * 51;
      random_number /= 6;
      int r = random_number % 6 * 51;
      random_number /= 6;
      int g = random_number % 6 * 51;
      random_number /= 6;
      int b = random_number % 6 * 51;
      SkPaint paint;
      paint.setColor(SkColorSetARGB(a, r, g, b));
      canvas->drawRect(
          SkRect::MakeXYWH(i * grid_size, j * grid_size, grid_size, grid_size),
          paint);
    }
  }
  scoped_refptr<PictureImageLayer> layer = PictureImageLayer::Create();
  layer->SetIsDrawable(true);
  layer->SetBounds(bounds);
  layer->SetImage(PaintImageBuilder()
                      .set_id(PaintImage::GetNextId())
                      .set_image(backing_store->makeImageSnapshot())
                      .TakePaintImage());
  return layer;
}

class LayerTreeHostMaskAsBlendingPixelTest
    : public LayerTreeHostPixelResourceTest,
      public ::testing::WithParamInterface<int> {
 public:
  LayerTreeHostMaskAsBlendingPixelTest()
      : LayerTreeHostPixelResourceTest(
            GetParam() ? GL_ZERO_COPY_RECT_DRAW : SOFTWARE,
            Layer::LayerMaskType::SINGLE_TEXTURE_MASK),
        use_antialiasing_(GetParam() && (GetParam() - 1) & 1),
        force_shaders_(GetParam() && (GetParam() - 1) & 2) {
    float percentage_pixels_small_error = 0.f;
    float percentage_pixels_error = 0.f;
    float average_error_allowed_in_bad_pixels = 0.f;
    int large_error_allowed = 0;
    int small_error_allowed = 0;
    if (use_antialiasing_) {
      percentage_pixels_small_error = 7.7f;
      percentage_pixels_error = 7.7f;
      average_error_allowed_in_bad_pixels = 1.7f;
      large_error_allowed = 13;
      small_error_allowed = 1;
    } else if (test_type_ != PIXEL_TEST_SOFTWARE) {
      percentage_pixels_small_error = 7.6f;
      percentage_pixels_error = 7.6f;
      average_error_allowed_in_bad_pixels = 1.7f;
      large_error_allowed = 13;
      small_error_allowed = 1;
    }

    pixel_comparator_.reset(new FuzzyPixelComparator(
        false,  // discard_alpha
        percentage_pixels_error, percentage_pixels_small_error,
        average_error_allowed_in_bad_pixels, large_error_allowed,
        small_error_allowed));
  }

 protected:
  std::unique_ptr<viz::TestLayerTreeFrameSink> CreateLayerTreeFrameSink(
      const viz::RendererSettings& renderer_settings,
      double refresh_rate,
      scoped_refptr<viz::ContextProvider> compositor_context_provider,
      scoped_refptr<viz::ContextProvider> worker_context_provider) override {
    viz::RendererSettings modified_renderer_settings = renderer_settings;
    modified_renderer_settings.force_antialiasing = use_antialiasing_;
    modified_renderer_settings.force_blending_with_shaders = force_shaders_;
    return LayerTreeHostPixelResourceTest::CreateLayerTreeFrameSink(
        modified_renderer_settings, refresh_rate, compositor_context_provider,
        worker_context_provider);
  }

  bool use_antialiasing_;
  bool force_shaders_;
};

INSTANTIATE_TEST_CASE_P(All,
                        LayerTreeHostMaskAsBlendingPixelTest,
                        ::testing::Range(0, 5));
// Instantiate 5 test modes of the following:
// 0: SOFTWARE (golden sample)
// 1: GL
// 2: GL + AA
// 3: GL + Forced Shaders
// 4: GL + Forced Shaders + AA

TEST_P(LayerTreeHostMaskAsBlendingPixelTest, PixelAlignedNoop) {
  // This test verifies the degenerate case of a no-op mask doesn't affect
  // the contents in any way.
  scoped_refptr<Layer> root = CreateCheckerboardLayer(gfx::Size(400, 300));

  scoped_refptr<Layer> mask_isolation = Layer::Create();
  mask_isolation->SetPosition(gfx::PointF(20, 20));
  mask_isolation->SetBounds(gfx::Size(350, 250));
  mask_isolation->SetMasksToBounds(true);
  mask_isolation->SetIsRootForIsolatedGroup(true);
  root->AddChild(mask_isolation);

  scoped_refptr<Layer> content =
      CreateRandomColorfulLayer(gfx::Size(400, 300), 25);
  content->SetPosition(gfx::PointF(-40, -40));
  mask_isolation->AddChild(content);

  scoped_refptr<Layer> mask_layer =
      CreateSolidColorLayer(gfx::Rect(350, 250), kCSSBlack);
  mask_layer->SetBlendMode(SkBlendMode::kDstIn);
  mask_isolation->AddChild(mask_layer);

  RunPixelResourceTest(
      root, base::FilePath(FILE_PATH_LITERAL("mask_as_blending_noop.png")));
}

TEST_P(LayerTreeHostMaskAsBlendingPixelTest, PixelAlignedClippedCircle) {
  // This test verifies a simple pixel aligned mask applies correctly.
  scoped_refptr<Layer> root = CreateCheckerboardLayer(gfx::Size(400, 300));

  scoped_refptr<Layer> mask_isolation = Layer::Create();
  mask_isolation->SetPosition(gfx::PointF(20, 20));
  mask_isolation->SetBounds(gfx::Size(350, 250));
  mask_isolation->SetMasksToBounds(true);
  mask_isolation->SetIsRootForIsolatedGroup(true);
  root->AddChild(mask_isolation);

  scoped_refptr<Layer> content =
      CreateRandomColorfulLayer(gfx::Size(400, 300), 25);
  content->SetPosition(gfx::PointF(-40, -40));
  mask_isolation->AddChild(content);

  scoped_refptr<PictureImageLayer> mask_layer = PictureImageLayer::Create();
  {
    sk_sp<SkSurface> backing_store = SkSurface::MakeRasterN32Premul(350, 250);
    SkCanvas* canvas = backing_store->getCanvas();
    SkPaint paint;
    paint.setColor(kCSSBlack);
    paint.setAntiAlias(true);
    canvas->drawCircle(175.f, 125.f, 180.f, paint);

    mask_layer->SetImage(PaintImageBuilder()
                             .set_id(PaintImage::GetNextId())
                             .set_image(backing_store->makeImageSnapshot())
                             .TakePaintImage());
  }
  mask_layer->SetIsDrawable(true);
  mask_layer->SetBounds(gfx::Size(350, 250));
  mask_layer->SetBlendMode(SkBlendMode::kDstIn);
  mask_isolation->AddChild(mask_layer);

  RunPixelResourceTest(
      root, base::FilePath(FILE_PATH_LITERAL("mask_as_blending_circle.png")));
}

TEST_P(LayerTreeHostMaskAsBlendingPixelTest,
       PixelAlignedClippedCircleUnderflow) {
  // This test verifies a simple pixel aligned mask applies correctly when
  // the content is smaller than the mask.
  scoped_refptr<Layer> root = CreateCheckerboardLayer(gfx::Size(400, 300));

  scoped_refptr<Layer> mask_isolation = Layer::Create();
  mask_isolation->SetPosition(gfx::PointF(20, 20));
  mask_isolation->SetBounds(gfx::Size(350, 250));
  mask_isolation->SetMasksToBounds(true);
  mask_isolation->SetIsRootForIsolatedGroup(true);
  root->AddChild(mask_isolation);

  scoped_refptr<Layer> content =
      CreateRandomColorfulLayer(gfx::Size(330, 230), 25);
  content->SetPosition(gfx::PointF(10, 10));
  mask_isolation->AddChild(content);

  scoped_refptr<PictureImageLayer> mask_layer = PictureImageLayer::Create();
  {
    sk_sp<SkSurface> backing_store = SkSurface::MakeRasterN32Premul(350, 250);
    SkCanvas* canvas = backing_store->getCanvas();
    SkPaint paint;
    paint.setColor(kCSSBlack);
    paint.setAntiAlias(true);
    canvas->drawCircle(175.f, 125.f, 180.f, paint);

    mask_layer->SetImage(PaintImageBuilder()
                             .set_id(PaintImage::GetNextId())
                             .set_image(backing_store->makeImageSnapshot())
                             .TakePaintImage());
  }
  mask_layer->SetIsDrawable(true);
  mask_layer->SetBounds(gfx::Size(350, 250));
  mask_layer->SetBlendMode(SkBlendMode::kDstIn);
  mask_isolation->AddChild(mask_layer);

  RunPixelResourceTest(root, base::FilePath(FILE_PATH_LITERAL(
                                 "mask_as_blending_circle_underflow.png")));
}

TEST_P(LayerTreeHostMaskAsBlendingPixelTest, RotatedClippedCircle) {
  // This test verifies a simple pixel aligned mask that is not pixel aligned
  // to its target surface is rendered correctly.
  scoped_refptr<Layer> root = CreateCheckerboardLayer(gfx::Size(400, 300));

  scoped_refptr<Layer> mask_isolation = Layer::Create();
  mask_isolation->SetPosition(gfx::PointF(20, 20));
  {
    gfx::Transform rotate;
    rotate.Rotate(5.f);
    mask_isolation->SetTransform(rotate);
  }
  mask_isolation->SetBounds(gfx::Size(350, 250));
  mask_isolation->SetMasksToBounds(true);
  mask_isolation->SetIsRootForIsolatedGroup(true);
  root->AddChild(mask_isolation);

  scoped_refptr<Layer> content =
      CreateRandomColorfulLayer(gfx::Size(400, 300), 25);
  content->SetPosition(gfx::PointF(-40, -40));
  mask_isolation->AddChild(content);

  scoped_refptr<PictureImageLayer> mask_layer = PictureImageLayer::Create();
  {
    sk_sp<SkSurface> backing_store = SkSurface::MakeRasterN32Premul(350, 250);
    SkCanvas* canvas = backing_store->getCanvas();
    SkPaint paint;
    paint.setColor(kCSSBlack);
    paint.setAntiAlias(true);
    canvas->drawCircle(175.f, 125.f, 180.f, paint);

    mask_layer->SetImage(PaintImageBuilder()
                             .set_id(PaintImage::GetNextId())
                             .set_image(backing_store->makeImageSnapshot())
                             .TakePaintImage());
  }
  mask_layer->SetIsDrawable(true);
  mask_layer->SetBounds(gfx::Size(350, 250));
  mask_layer->SetBlendMode(SkBlendMode::kDstIn);
  mask_isolation->AddChild(mask_layer);

  RunPixelResourceTest(
      root,
      base::FilePath(FILE_PATH_LITERAL("mask_as_blending_rotated_circle.png")));
}

TEST_P(LayerTreeHostMaskAsBlendingPixelTest, RotatedClippedCircleUnderflow) {
  // This test verifies a simple pixel aligned mask that is not pixel aligned
  // to its target surface, and has the content smaller than the mask, is
  // rendered correctly.
  scoped_refptr<Layer> root = CreateCheckerboardLayer(gfx::Size(400, 300));

  scoped_refptr<Layer> mask_isolation = Layer::Create();
  mask_isolation->SetPosition(gfx::PointF(20, 20));
  {
    gfx::Transform rotate;
    rotate.Rotate(5.f);
    mask_isolation->SetTransform(rotate);
  }
  mask_isolation->SetBounds(gfx::Size(350, 250));
  mask_isolation->SetMasksToBounds(true);
  mask_isolation->SetIsRootForIsolatedGroup(true);
  root->AddChild(mask_isolation);

  scoped_refptr<Layer> content =
      CreateRandomColorfulLayer(gfx::Size(330, 230), 25);
  content->SetPosition(gfx::PointF(10, 10));
  mask_isolation->AddChild(content);

  scoped_refptr<PictureImageLayer> mask_layer = PictureImageLayer::Create();
  {
    sk_sp<SkSurface> backing_store = SkSurface::MakeRasterN32Premul(350, 250);
    SkCanvas* canvas = backing_store->getCanvas();
    SkPaint paint;
    paint.setColor(kCSSBlack);
    paint.setAntiAlias(true);
    canvas->drawCircle(175.f, 125.f, 180.f, paint);

    mask_layer->SetImage(PaintImageBuilder()
                             .set_id(PaintImage::GetNextId())
                             .set_image(backing_store->makeImageSnapshot())
                             .TakePaintImage());
  }
  mask_layer->SetIsDrawable(true);
  mask_layer->SetBounds(gfx::Size(350, 250));
  mask_layer->SetBlendMode(SkBlendMode::kDstIn);
  mask_isolation->AddChild(mask_layer);

  RunPixelResourceTest(root,
                       base::FilePath(FILE_PATH_LITERAL(
                           "mask_as_blending_rotated_circle_underflow.png")));
}

}  // namespace
}  // namespace cc

#endif  // OS_ANDROID

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/command_line.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/base/region.h"
#include "cc/layers/recording_source.h"
#include "cc/paint/display_item_list.h"
#include "cc/raster/raster_source.h"
#include "cc/test/pixel_test_utils.h"
#include "cc/test/test_in_process_context_provider.h"
#include "gpu/command_buffer/client/gles2_implementation.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/shared_memory_limits.h"
#include "gpu/command_buffer/common/gles2_cmd_utils.h"
#include "gpu/config/gpu_switches.h"
#include "gpu/ipc/gl_in_process_context.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/gpu/GrTypes.h"
#include "ui/gfx/geometry/axis_transform2d.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/skia_util.h"

namespace cc {
namespace {

class OopPixelTest : public testing::Test {
 public:
  void SetUp() override {
    bool is_offscreen = true;
    gpu::gles2::ContextCreationAttribHelper attribs;
    attribs.alpha_size = -1;
    attribs.depth_size = 24;
    attribs.stencil_size = 8;
    attribs.samples = 0;
    attribs.sample_buffers = 0;
    attribs.fail_if_major_perf_caveat = false;
    attribs.bind_generates_resource = false;
    // Enable OOP rasterization.
    attribs.enable_oop_rasterization = true;

    // Add an OOP rasterization command line flag so that we set
    // |chromium_raster_transport| features flag.
    // TODO(vmpstr): Is there a better way to do this?
    if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kEnableOOPRasterization)) {
      base::CommandLine::ForCurrentProcess()->AppendSwitch(
          switches::kEnableOOPRasterization);
    }

    context_ = gpu::GLInProcessContext::CreateWithoutInit();
    auto result = context_->Initialize(
        nullptr, nullptr, is_offscreen, gpu::kNullSurfaceHandle, nullptr,
        attribs, gpu::SharedMemoryLimits(), &gpu_memory_buffer_manager_,
        &image_factory_, base::ThreadTaskRunnerHandle::Get());

    ASSERT_EQ(result, gpu::ContextResult::kSuccess);
  }

  void TearDown() override { context_.reset(); }

  struct RasterOptions {
    SkColor background_color = SK_ColorBLACK;
    int msaa_sample_count = 0;
    bool use_lcd_text = false;
    bool use_distance_field_text = false;
    GrPixelConfig pixel_config = kRGBA_8888_GrPixelConfig;
    gfx::Rect bitmap_rect;
    gfx::Rect playback_rect;
    float post_translate_x = 0.f;
    float post_translate_y = 0.f;
    float post_scale = 1.f;
  };

  std::vector<SkColor> Raster(scoped_refptr<DisplayItemList> display_item_list,
                              const gfx::Size& playback_size) {
    RasterOptions options;
    options.bitmap_rect = gfx::Rect(playback_size);
    options.playback_rect = gfx::Rect(playback_size);
    return Raster(display_item_list, options);
  }

  std::vector<SkColor> Raster(scoped_refptr<DisplayItemList> display_item_list,
                              const RasterOptions& options) {
    gpu::gles2::GLES2Interface* gl = context_->GetImplementation();
    GLuint texture_id;
    GLuint fbo_id;
    int width = options.bitmap_rect.width();
    int height = options.bitmap_rect.height();

    // Create and allocate a texture.
    gl->GenTextures(1, &texture_id);
    gl->BindTexture(GL_TEXTURE_2D, texture_id);
    std::vector<uint8_t> pixels;
    pixels.resize(width * height * 4);
    for (size_t i = 0; i < pixels.size(); ++i)
      pixels[i] = 128;
    gl->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, &pixels[0]);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Create an fbo and bind the texture to it.
    gl->GenFramebuffers(1, &fbo_id);
    gl->BindFramebuffer(GL_FRAMEBUFFER, fbo_id);
    gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, texture_id, 0);
    DCHECK(gl->CheckFramebufferStatus(GL_FRAMEBUFFER) ==
           GL_FRAMEBUFFER_COMPLETE);

    gl->ClearColor(1.f, 0.f, 1.f, 1.f);
    gl->Clear(GL_COLOR_BUFFER_BIT);
    gl->Flush();

    EXPECT_EQ(gl->GetError(), static_cast<unsigned>(GL_NO_ERROR));

    // Read the data back.
    std::unique_ptr<unsigned char[]> data(
        new unsigned char[width * height * 4]);
    gl->ReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data.get());

    gl->DeleteTextures(1, &texture_id);
    gl->DeleteFramebuffers(1, &fbo_id);

    std::vector<SkColor> colors;
    colors.reserve(width * height);
    for (int h = 0; h < height; ++h) {
      for (int w = 0; w < width; ++w) {
        int i = (h * width + w) * 4;
        colors.push_back(
            SkColorSetARGB(data[i + 3], data[i + 0], data[i + 1], data[i + 2]));
      }
    }
    fprintf(stderr, "enne: %d, %d, %d, %d\n", data[0], data[1], data[2],
            data[3]);
    return colors;
  }

  std::vector<SkColor> RasterExpectedBitmap(
      scoped_refptr<DisplayItemList> display_item_list,
      const gfx::Size& playback_size) {
    RasterOptions options;
    options.bitmap_rect = gfx::Rect(playback_size);
    options.playback_rect = gfx::Rect(playback_size);
    return RasterExpectedBitmap(display_item_list, options);
  }

  std::vector<SkColor> RasterExpectedBitmap(
      scoped_refptr<DisplayItemList> display_item_list,
      const RasterOptions& options) {
    // Not all oop raster options are valid for the bitmap path.
    EXPECT_EQ(0, options.msaa_sample_count);
    EXPECT_FALSE(options.use_distance_field_text);
    EXPECT_EQ(kRGBA_8888_GrPixelConfig, options.pixel_config);

    // Generate bitmap via the "in process" raster path.  This verifies
    // that the preamble setup in RasterSource::PlaybackToCanvas matches
    // the same setup done in GLES2Implementation::RasterCHROMIUM.
    RecordingSource recording;
    recording.UpdateDisplayItemList(display_item_list, 0u);
    recording.SetBackgroundColor(options.background_color);
    Region fake_invalidation;
    gfx::Rect layer_rect(
        gfx::Size(options.bitmap_rect.right(), options.bitmap_rect.bottom()));
    recording.UpdateAndExpandInvalidation(&fake_invalidation, layer_rect.size(),
                                          layer_rect);

    auto raster_source = recording.CreateRasterSource();
    RasterSource::PlaybackSettings settings;
    settings.use_lcd_text = options.use_lcd_text;
    // OOP raster does not support the complicated debug color clearing from
    // RasterSource::ClearCanvasForPlayback, so disable it for consistency.
    settings.clear_canvas_before_raster = false;
    // TODO(enne): add a fake image provider here?

    // TODO(enne): Does this need to use Ganesh for 100% pixel consistency?
    SkBitmap bitmap;
    SkImageInfo info = SkImageInfo::Make(
        options.bitmap_rect.width(), options.bitmap_rect.height(),
        SkColorType::kBGRA_8888_SkColorType, SkAlphaType::kPremul_SkAlphaType);
    bitmap.allocPixels(info, options.bitmap_rect.width() * 4);
    SkCanvas canvas(bitmap);
    // One difference with out of process raster is that the bitmap rect
    // can't be trusted with respect to clearing, so it clears everything.
    canvas.drawColor(options.background_color);

    gfx::AxisTransform2d raster_transform(
        options.post_scale,
        gfx::Vector2dF(options.post_translate_x, options.post_translate_y));
    // TODO(enne): add a target colorspace to BeginRasterCHROMIUM etc.
    gfx::ColorSpace target_color_space;
    raster_source->PlaybackToCanvas(&canvas, target_color_space,
                                    options.bitmap_rect, options.playback_rect,
                                    raster_transform, settings);
    bitmap.setImmutable();

    int width = options.bitmap_rect.width();
    int height = options.bitmap_rect.height();
    unsigned char* data = static_cast<unsigned char*>(bitmap.getPixels());
    CHECK(data);

    std::vector<SkColor> colors;
    colors.reserve(width * height);
    for (int h = 0; h < height; ++h) {
      for (int w = 0; w < width; ++w) {
        int i = (h * width + w) * 4;
        colors.push_back(
            SkColorSetARGB(data[i + 3], data[i + 2], data[i + 1], data[i + 0]));
      }
    }
    return colors;
  }

  void ExpectEquals(const gfx::Size& size,
                    std::vector<SkColor>* actual,
                    std::vector<SkColor>* expected) {
    size_t expected_size = size.width() * size.height();
    ASSERT_EQ(expected->size(), expected_size);
    if (*actual == *expected)
      return;

    ASSERT_EQ(actual->size(), expected_size);

    SkBitmap expected_bitmap;
    expected_bitmap.installPixels(
        SkImageInfo::MakeN32Premul(size.width(), size.height()),
        expected->data(), size.width() * sizeof(SkColor));

    SkBitmap actual_bitmap;
    actual_bitmap.installPixels(
        SkImageInfo::MakeN32Premul(size.width(), size.height()), actual->data(),
        size.width() * sizeof(SkColor));

    ADD_FAILURE() << "\nExpected: " << GetPNGDataUrl(expected_bitmap)
                  << "\nActual:   " << GetPNGDataUrl(actual_bitmap);
  }

 private:
  viz::TestGpuMemoryBufferManager gpu_memory_buffer_manager_;
  TestImageFactory image_factory_;
  std::unique_ptr<gpu::GLInProcessContext> context_;
};

TEST_F(OopPixelTest, DrawColor) {
  gfx::Rect rect(10, 10);
  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  display_item_list->push<DrawColorOp>(SK_ColorBLUE, SkBlendMode::kSrc);
  display_item_list->EndPaintOfUnpaired(rect);
  display_item_list->Finalize();

  auto actual = Raster(std::move(display_item_list), rect.size());
  std::vector<SkColor> expected(rect.width() * rect.height(),
                                SkColorSetARGB(255, 0, 0, 255));
  ExpectEquals(rect.size(), &actual, &expected);
}

TEST_F(OopPixelTest, DrawRect) {
  gfx::Rect rect(10, 10);
  auto color_paint = [](int r, int g, int b) {
    PaintFlags flags;
    flags.setColor(SkColorSetARGB(255, r, g, b));
    return flags;
  };
  std::vector<std::pair<SkRect, PaintFlags>> input = {
      {SkRect::MakeXYWH(0, 0, 5, 5), color_paint(0, 0, 255)},
      {SkRect::MakeXYWH(5, 0, 5, 5), color_paint(0, 255, 0)},
      {SkRect::MakeXYWH(0, 5, 5, 5), color_paint(0, 255, 255)},
      {SkRect::MakeXYWH(5, 5, 5, 5), color_paint(255, 0, 0)}};

  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  for (auto& op : input) {
    display_item_list->StartPaint();
    display_item_list->push<DrawRectOp>(op.first, op.second);
    display_item_list->EndPaintOfUnpaired(
        gfx::ToEnclosingRect(gfx::SkRectToRectF(op.first)));
  }
  display_item_list->Finalize();

  auto actual = Raster(std::move(display_item_list), rect.size());

  // Expected colors are 5x5 rects of
  //  BLUE GREEN
  //  CYAN  RED
  std::vector<SkColor> expected(rect.width() * rect.height());
  for (int h = 0; h < rect.height(); ++h) {
    auto start = expected.begin() + h * rect.width();
    SkColor left_color =
        h < 5 ? input[0].second.getColor() : input[2].second.getColor();
    SkColor right_color =
        h < 5 ? input[1].second.getColor() : input[3].second.getColor();

    std::fill(start, start + 5, left_color);
    std::fill(start + 5, start + 10, right_color);
  }
  ExpectEquals(rect.size(), &actual, &expected);
}

// Test various bitmap and playback rects in the raster options, to verify
// that in process (RasterSource) and out of process (GLES2Implementation)
// raster behave identically.
TEST_F(OopPixelTest, DrawRectBasicRasterOptions) {
  PaintFlags flags;
  flags.setColor(SkColorSetARGB(255, 250, 10, 20));
  gfx::Rect draw_rect(3, 1, 8, 9);

  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  display_item_list->push<DrawRectOp>(gfx::RectToSkRect(draw_rect), flags);
  display_item_list->EndPaintOfUnpaired(draw_rect);
  display_item_list->Finalize();

  std::vector<std::pair<gfx::Rect, gfx::Rect>> input = {
      {{0, 0, 10, 10}, {0, 0, 10, 10}},
      {{1, 2, 10, 10}, {4, 2, 5, 6}},
      {{5, 5, 15, 10}, {0, 0, 10, 10}}};

  for (size_t i = 0; i < input.size(); ++i) {
    SCOPED_TRACE(base::StringPrintf("Case %zd", i));

    RasterOptions options;
    options.bitmap_rect = input[i].first;
    options.playback_rect = input[i].second;
    options.background_color = SK_ColorMAGENTA;

    auto actual = Raster(display_item_list, options);
    auto expected = RasterExpectedBitmap(display_item_list, options);
    ExpectEquals(options.bitmap_rect.size(), &actual, &expected);
  }
}

}  // namespace
}  // namespace cc

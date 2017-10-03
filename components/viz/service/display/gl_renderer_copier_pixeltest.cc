// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/gl_renderer_copier.h"

#include <stdint.h>

#include <cstring>
#include <memory>
#include <tuple>

#include "base/bind.h"
#include "base/run_loop.h"
#include "cc/test/pixel_test.h"
#include "cc/test/pixel_test_utils.h"
#include "components/viz/common/frame_sinks/copy_output_request.h"
#include "components/viz/common/frame_sinks/copy_output_result.h"
#include "components/viz/service/display/gl_renderer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkPixmap.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace viz {

namespace {

// Possible combinations of scenarios GLRendererCopier will encounter, which
// will cause it to follow different workflows. The cartesian product of these
// will be individually tested to ensure all possible workflows are tested.
enum SourceFormat {
  kForRGBA = 0,
  kForRGB = 1,
};
enum SourceObjects {
  kOnlyHaveFramebuffer = 0,
  kHaveTextureToo = 1,
};
enum ResultType {
  kSkBitmapResult = 0,
  kTextureResult = 1,
};
enum ScaleResult {
  kNoScaling = 0,
  kScaleByHalf = 1,
};

// The size of the source texture or framebuffer.
constexpr gfx::Size kSourceSize = gfx::Size(240, 120);

// In order to test coordinate calculations and Y-flipping, the tests will issue
// copy requests for a small region just to the right and below the center of
// the entire source texture/framebuffer.
constexpr gfx::Rect kRequestArea = gfx::Rect(kSourceSize.width() * 2 / 4,
                                             kSourceSize.height() * 2 / 4,
                                             kSourceSize.width() / 4,
                                             kSourceSize.height() / 4);

}  // namespace

class GLRendererCopierPixelTest
    : public cc::GLRendererPixelTest,
      public testing::WithParamInterface<
          std::tuple<SourceFormat, SourceObjects, ResultType, ScaleResult>> {
 public:
  void SetUp() override {
    cc::GLRendererPixelTest::SetUp();
    gl_ = renderer()->gl_;
  }

  void TearDown() override {
    DeleteSourceFramebuffer();
    DeleteSourceTexture();
    cc::GLRendererPixelTest::TearDown();
  }

  GLRendererCopier* copier() { return &(renderer()->copier_); }

  GLenum gl_format() const {
    return static_cast<GLenum>((std::get<0>(GetParam()) == kForRGBA) ? GL_RGBA
                                                                     : GL_RGB);
  }

  // Creates a packed RGBA (bytes_per_pixel=4) or RGB (bytes_per_pixel=3) bitmap
  // in OpenGL byte/row order. The pattern consists of a grid of 16 distinct
  // rects of color.
  std::unique_ptr<uint8_t[]> CreateSourcePattern(int bytes_per_pixel) {
    const int stride = kSourceSize.width() * bytes_per_pixel;
    std::unique_ptr<uint8_t[]> pixels(
        new uint8_t[stride * kSourceSize.height()]);
    for (int y = 0; y < kSourceSize.height(); ++y) {
      const int flipped_y = kSourceSize.height() - y - 1;
      int byte_offset = flipped_y * stride;
      for (int x = 0; x < kSourceSize.width();
           ++x, byte_offset += bytes_per_pixel) {
        const int x_color_step = x / (kSourceSize.width() / 4);
        const int y_color_step = y / (kSourceSize.height() / 4);
        pixels[byte_offset + 0] = 63 * x_color_step;
        pixels[byte_offset + 1] = 63 * y_color_step;
        pixels[byte_offset + 2] = 63;
        if (bytes_per_pixel == 4)
          pixels[byte_offset + 3] = 255;
      }
    }
    return pixels;
  }

  // Returns an SkBitmap with Skia's native byte ordering, given a packed RGBA
  // bitmap in OpenGL byte/row order.
  SkBitmap CreateSkBitmapFromGLPixels(const uint8_t* pixels,
                                      const gfx::Size& size) {
    // Copy pixels into an identically-formatted SkBitmap, but with a Y-flip.
    SkBitmap bitmap;
    bitmap.allocPixels(
        SkImageInfo::Make(size.width(), size.height(), kRGBA_8888_SkColorType,
                          kPremul_SkAlphaType),
        size.width() * 4);
    for (int y = 0; y < size.height(); ++y) {
      const int flipped_y = size.height() - y - 1;
      const uint8_t* const src_row = pixels + flipped_y * size.width() * 4;
      void* const dest_row = bitmap.getAddr(0, y);
      std::memcpy(dest_row, src_row, size.width() * 4);
    }

    // Convert to Skia's native byte ordering.
    SkBitmap native_bitmap;
    native_bitmap.allocN32Pixels(bitmap.width(), bitmap.height());
    SkPixmap pixmap;
    CHECK(bitmap.peekPixels(&pixmap));
    CHECK(native_bitmap.writePixels(pixmap, 0, 0));
    return native_bitmap;
  }

  GLuint CreateSourceTexture() {
    CHECK_EQ(0u, source_texture_);
    gl_->GenTextures(1, &source_texture_);
    gl_->BindTexture(GL_TEXTURE_2D, source_texture_);
    gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl_->TexImage2D(GL_TEXTURE_2D, 0, gl_format(), kSourceSize.width(),
                    kSourceSize.height(), 0, gl_format(), GL_UNSIGNED_BYTE,
                    CreateSourcePattern(gl_format() == GL_RGBA ? 4 : 3).get());
    gl_->BindTexture(GL_TEXTURE_2D, 0);
    return source_texture_;
  }

  void DeleteSourceTexture() {
    if (source_texture_ != 0) {
      gl_->DeleteTextures(1, &source_texture_);
      source_texture_ = 0;
    }
  }

  void CreateAndBindSourceFramebuffer(GLuint texture) {
    ASSERT_EQ(0u, source_framebuffer_);
    gl_->GenFramebuffers(1, &source_framebuffer_);
    gl_->BindFramebuffer(GL_FRAMEBUFFER, source_framebuffer_);
    gl_->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_2D, texture, 0);
  }

  void DeleteSourceFramebuffer() {
    if (source_framebuffer_ != 0) {
      gl_->DeleteFramebuffers(1, &source_framebuffer_);
      source_framebuffer_ = 0;
    }
  }

  // Creates a SkBitmap that contains the expected copy output result image.
  SkBitmap CreateExpectedSkBitmap() {
    const SkBitmap source =
        CreateSkBitmapFromGLPixels(CreateSourcePattern(4).get(), kSourceSize);
    SkIRect source_rect{kRequestArea.x(), kRequestArea.y(),
                        kRequestArea.right(), kRequestArea.bottom()};
    if (std::get<3>(GetParam()) == kScaleByHalf) {
      // This is cheating, but it gives the same pixels as a true scaling. ;-)
      source_rect.fRight -= kRequestArea.width() / 2;
      source_rect.fBottom -= kRequestArea.height() / 2;
    }
    SkBitmap result;
    CHECK(source.extractSubset(&result, source_rect));
    return result;
  }

  // Reads back the texture in the given |mailbox| to a SkBitmap in Skia-native
  // format.
  SkBitmap ReadbackToSkBitmap(const TextureMailbox& mailbox,
                              const gfx::Size& texture_size) {
    // Bind the texture to a framebuffer from which to read the pixels.
    if (mailbox.sync_token().HasData())
      gl_->WaitSyncTokenCHROMIUM(mailbox.sync_token().GetConstData());
    GLuint texture =
        gl_->CreateAndConsumeTextureCHROMIUM(mailbox.target(), mailbox.name());
    GLuint framebuffer = 0;
    gl_->GenFramebuffers(1, &framebuffer);
    gl_->BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    gl_->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_2D, texture, 0);

    // Read the pixels and convert to SkBitmap form for test comparisons.
    std::unique_ptr<uint8_t[]> pixels(new uint8_t[texture_size.GetArea() * 4]);
    gl_->ReadPixels(0, 0, texture_size.width(), texture_size.height(), GL_RGBA,
                    GL_UNSIGNED_BYTE, pixels.get());
    gl_->DeleteFramebuffers(1, &framebuffer);
    gl_->DeleteTextures(1, &texture);
    return CreateSkBitmapFromGLPixels(pixels.get(), texture_size);
  }

 private:
  gpu::gles2::GLES2Interface* gl_ = nullptr;
  GLuint source_texture_ = 0;
  GLuint source_framebuffer_ = 0;
};

TEST_P(GLRendererCopierPixelTest, ExecutesCopyRequest) {
  // Create and execute a CopyOutputRequest via the GLRendererCopier.
  std::unique_ptr<CopyOutputResult> result;
  {
    base::RunLoop loop;
    std::unique_ptr<CopyOutputRequest> request(new CopyOutputRequest(
        (std::get<2>(GetParam()) == kSkBitmapResult)
            ? CopyOutputRequest::ResultFormat::RGBA_BITMAP
            : CopyOutputRequest::ResultFormat::RGBA_TEXTURE,
        base::BindOnce(
            [](std::unique_ptr<CopyOutputResult>* result,
               const base::Closure& quit_closure,
               std::unique_ptr<CopyOutputResult> result_from_copier) {
              *result = std::move(result_from_copier);
              quit_closure.Run();
            },
            &result, loop.QuitClosure())));
    request->set_area(kRequestArea);
    if (std::get<3>(GetParam()) == kScaleByHalf) {
      request->SetUniformScaleRatio(2, 1);
    }

    const GLuint source_texture = CreateSourceTexture();
    CreateAndBindSourceFramebuffer(source_texture);
    // This is equivalent to MoveFromDrawToWindowSpace(request->area()):
    const gfx::Rect copy_rect(kRequestArea.x(),
                              kSourceSize.height() - kRequestArea.bottom(),
                              kRequestArea.width(), kRequestArea.height());
    const gfx::ColorSpace color_space = gfx::ColorSpace::CreateSRGB();
    if (std::get<1>(GetParam()) == kHaveTextureToo) {
      copier()->CopyFromFramebuffer(std::move(request), copy_rect, gl_format(),
                                    source_texture, kSourceSize, color_space);
    } else {
      copier()->CopyFromFramebuffer(std::move(request), copy_rect, gl_format(),
                                    0, gfx::Size(), color_space);
    }
    loop.Run();
  }

  // Check that a result was produced and is of the expected rect/size.
  ASSERT_TRUE(result);
  ASSERT_FALSE(result->IsEmpty());
  if (std::get<3>(GetParam()) == kNoScaling)
    ASSERT_EQ(kRequestArea, result->rect());
  else
    ASSERT_EQ(gfx::ScaleToEnclosingRect(kRequestArea, 0.5f), result->rect());

  // Examine the image in the |result|.
  const SkBitmap expected = CreateExpectedSkBitmap();
  const SkBitmap actual =
      (std::get<2>(GetParam()) == kSkBitmapResult)
          ? result->AsSkBitmap()
          : ReadbackToSkBitmap(*result->GetTextureMailbox(), result->size());
  ASSERT_EQ(expected.info(), actual.info());
  for (int row = 0; row < expected.height(); ++row) {
    const int compare_result = std::memcmp(
        expected.getAddr(0, row), actual.getAddr(0, row), expected.width() * 4);
    EXPECT_EQ(0, compare_result)
        << "First difference at row " << row
        << "\nExpected bitmap: " << cc::GetPNGDataUrl(expected)
        << "\nActual bitmap: " << cc::GetPNGDataUrl(actual)
        << "\nWhole source: "
        << cc::GetPNGDataUrl(CreateSkBitmapFromGLPixels(
               CreateSourcePattern(4).get(), kSourceSize));
    if (HasFailure())
      break;
  }
}

INSTANTIATE_TEST_CASE_P(
    ,
    GLRendererCopierPixelTest,
    testing::Combine(testing::Values(kForRGBA, kForRGB),
                     testing::Values(kOnlyHaveFramebuffer, kHaveTextureToo),
                     testing::Values(kSkBitmapResult, kTextureResult),
                     testing::Values(kNoScaling, kScaleByHalf)));

}  // namespace viz

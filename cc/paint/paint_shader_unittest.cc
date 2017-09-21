// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_shader.h"

#include "cc/paint/draw_image.h"
#include "cc/paint/image_provider.h"
#include "cc/paint/paint_image_builder.h"
#include "cc/paint/paint_op_buffer.h"
#include "cc/test/skia_common.h"
#include "cc/test/stub_paint_image_generator.h"
#include "cc/test/test_skcanvas.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

class MockImageGenerator : public StubPaintImageGenerator {
 public:
  explicit MockImageGenerator(const gfx::Size& size)
      : StubPaintImageGenerator(
            SkImageInfo::MakeN32Premul(size.width(), size.height())) {}

  MOCK_METHOD5(GetPixels,
               bool(const SkImageInfo&, void*, size_t, size_t, uint32_t));
};

class MockImageProvider : public ImageProvider {
 public:
  MockImageProvider() = default;
  ~MockImageProvider() override = default;

  ScopedDecodedDrawImage GetDecodedDrawImage(const PaintImage& paint_image,
                                             const SkRect& src_rect,
                                             SkFilterQuality filter_quality,
                                             const SkMatrix& matrix) override {
    paint_image_ = paint_image;
    matrix_ = matrix;

    SkBitmap bitmap;
    bitmap.allocN32Pixels(10, 10);
    sk_sp<SkImage> image = SkImage::MakeFromBitmap(bitmap);
    return ScopedDecodedDrawImage(DecodedDrawImage(
        image, SkSize::MakeEmpty(), SkSize::Make(1.0f, 1.0f), filter_quality));
  }

  const PaintImage& paint_image() const { return paint_image_; }
  const SkMatrix& matrix() const { return matrix_; }

 private:
  PaintImage paint_image_;
  SkMatrix matrix_;
};

}  // namespace

TEST(PaintShaderTest, RasterizationRectForRecordShaders) {
  SkMatrix local_matrix = SkMatrix::MakeScale(0.5f, 0.5f);
  auto record_shader = PaintShader::MakePaintRecord(
      sk_make_sp<PaintOpBuffer>(), SkRect::MakeWH(100, 100),
      SkShader::TileMode::kClamp_TileMode, SkShader::TileMode::kClamp_TileMode,
      &local_matrix);

  SkRect tile_rect;
  SkMatrix ctm = SkMatrix::MakeScale(0.5f, 0.5f);
  EXPECT_TRUE(record_shader->GetRasterizationTileRect(ctm, &tile_rect));
  EXPECT_EQ(tile_rect, SkRect::MakeWH(25, 25));
}

TEST(PaintShaderTest, DecodePaintRecord) {
  auto record = sk_make_sp<PaintOpBuffer>();

  // Use a strict mock for the generator. It should never be used when
  // rasterizing this shader, since the decode should be done by the
  // ImageProvider.
  auto generator =
      sk_make_sp<testing::StrictMock<MockImageGenerator>>(gfx::Size(100, 100));
  PaintImage paint_image = PaintImageBuilder()
                               .set_id(PaintImage::GetNextId())
                               .set_paint_image_generator(generator)
                               .TakePaintImage();

  record->push<DrawImageOp>(paint_image, 0.f, 0.f, nullptr);
  SkMatrix local_matrix = SkMatrix::MakeScale(0.5f, 0.5f);
  auto record_shader = PaintShader::MakePaintRecord(
      record, SkRect::MakeWH(100, 100), SkShader::TileMode::kClamp_TileMode,
      SkShader::TileMode::kClamp_TileMode, &local_matrix);

  PaintOpBuffer buffer;
  PaintFlags flags;
  flags.setShader(record_shader);
  buffer.push<ScaleOp>(0.5f, 0.5f);
  buffer.push<DrawRectOp>(SkRect::MakeWH(100, 100), flags);

  MockImageProvider image_provider;
  SaveCountingCanvas canvas;
  buffer.Playback(&canvas, &image_provider);

  EXPECT_EQ(canvas.draw_rect_, SkRect::MakeWH(100, 100));
  SkShader* shader = canvas.paint_.getShader();
  ASSERT_TRUE(shader);
  SkMatrix decoded_local_matrix;
  SkShader::TileMode xy[2];
  SkImage* skia_image = shader->isAImage(&decoded_local_matrix, xy);
  ASSERT_TRUE(skia_image);
  EXPECT_TRUE(skia_image->isLazyGenerated());
  EXPECT_EQ(xy[0], record_shader->tx());
  EXPECT_EQ(xy[1], record_shader->ty());
  EXPECT_EQ(decoded_local_matrix, SkMatrix::MakeScale(2.f, 2.f));

  // Using the shader requests decode for images at the correct scale.
  EXPECT_EQ(image_provider.paint_image(), paint_image);
  SkSize size;
  image_provider.matrix().decomposeScale(&size);
  EXPECT_EQ(image_provider.matrix(), SkMatrix::MakeScale(0.25f, 0.25f));
}

}  // namespace cc

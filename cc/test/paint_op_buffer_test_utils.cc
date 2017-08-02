// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "cc/test/paint_op_buffer_test_utils.h"

#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "cc/paint/decoded_draw_image.h"
#include "cc/paint/display_item_list.h"
#include "cc/paint/image_provider.h"
#include "cc/paint/paint_op_buffer.h"
#include "cc/paint/paint_op_reader.h"
#include "cc/paint/paint_op_writer.h"
#include "cc/test/paint_op_buffer_test_utils.h"
#include "cc/test/skia_common.h"
#include "cc/test/test_skcanvas.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkFlattenableSerialization.h"
#include "third_party/skia/include/core/SkWriteBuffer.h"
#include "third_party/skia/include/effects/SkBlurMaskFilter.h"
#include "third_party/skia/include/effects/SkColorMatrixFilter.h"
#include "third_party/skia/include/effects/SkDashPathEffect.h"
#include "third_party/skia/include/effects/SkLayerDrawLooper.h"
#include "third_party/skia/include/effects/SkOffsetImageFilter.h"

namespace cc {
namespace {

std::vector<float> test_floats = {0.f,
                                  1.f,
                                  -1.f,
                                  2384.981971f,
                                  0.0001f,
                                  std::numeric_limits<float>::min(),
                                  std::numeric_limits<float>::max(),
                                  std::numeric_limits<float>::infinity()};

std::vector<uint8_t> test_uint8s = {
    0, 255, 128, 10, 45,
};

std::vector<SkRect> test_rects = {
    SkRect::MakeXYWH(1, 2.5, 3, 4), SkRect::MakeXYWH(0, 0, 0, 0),
    SkRect::MakeLargest(),          SkRect::MakeXYWH(0.5f, 0.5f, 8.2f, 8.2f),
    SkRect::MakeXYWH(-1, -1, 0, 0), SkRect::MakeXYWH(-100, -101, -102, -103)};

std::vector<SkRRect> test_rrects = {
    SkRRect::MakeEmpty(), SkRRect::MakeOval(SkRect::MakeXYWH(1, 2, 3, 4)),
    SkRRect::MakeRect(SkRect::MakeXYWH(-10, 100, 5, 4)),
    [] {
      SkRRect rrect = SkRRect::MakeEmpty();
      rrect.setNinePatch(SkRect::MakeXYWH(10, 20, 30, 40), 1, 2, 3, 4);
      return rrect;
    }(),
};

std::vector<SkIRect> test_irects = {
    SkIRect::MakeXYWH(1, 2, 3, 4),   SkIRect::MakeXYWH(0, 0, 0, 0),
    SkIRect::MakeLargest(),          SkIRect::MakeXYWH(0, 0, 10, 10),
    SkIRect::MakeXYWH(-1, -1, 0, 0), SkIRect::MakeXYWH(-100, -101, -102, -103)};

std::vector<SkMatrix> test_matrices = {
    SkMatrix(),
    SkMatrix::MakeScale(3.91f, 4.31f),
    SkMatrix::MakeTrans(-5.2f, 8.7f),
    [] {
      SkMatrix matrix;
      SkScalar buffer[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
      matrix.set9(buffer);
      return matrix;
    }(),
    [] {
      SkMatrix matrix;
      SkScalar buffer[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
      matrix.set9(buffer);
      return matrix;
    }(),
};

std::vector<SkPath> test_paths = {
    [] {
      SkPath path;
      path.moveTo(SkIntToScalar(20), SkIntToScalar(20));
      path.lineTo(SkIntToScalar(80), SkIntToScalar(20));
      path.lineTo(SkIntToScalar(30), SkIntToScalar(30));
      path.lineTo(SkIntToScalar(20), SkIntToScalar(80));
      return path;
    }(),
    [] {
      SkPath path;
      path.addCircle(2, 2, 5);
      path.addCircle(3, 4, 2);
      path.addArc(SkRect::MakeXYWH(1, 2, 3, 4), 5, 6);
      return path;
    }(),
    SkPath(),
};

std::vector<PaintFlags> test_flags = {
    PaintFlags(),
    [] {
      PaintFlags flags;
      flags.setTextSize(82.7f);
      flags.setColor(SK_ColorMAGENTA);
      flags.setStrokeWidth(4.2f);
      flags.setStrokeMiter(5.91f);
      flags.setBlendMode(SkBlendMode::kDst);
      flags.setStrokeCap(PaintFlags::kSquare_Cap);
      flags.setStrokeJoin(PaintFlags::kBevel_Join);
      flags.setStyle(PaintFlags::kStrokeAndFill_Style);
      flags.setTextEncoding(PaintFlags::kGlyphID_TextEncoding);
      flags.setHinting(PaintFlags::kNormal_Hinting);
      flags.setFilterQuality(SkFilterQuality::kMedium_SkFilterQuality);
      flags.setShader(PaintShader::MakeColor(SkColorSetARGB(1, 2, 3, 4)));
      return flags;
    }(),
    [] {
      PaintFlags flags;
      flags.setTextSize(0.0f);
      flags.setColor(SK_ColorCYAN);
      flags.setAlpha(103);
      flags.setStrokeWidth(0.32f);
      flags.setStrokeMiter(7.98f);
      flags.setBlendMode(SkBlendMode::kSrcOut);
      flags.setStrokeCap(PaintFlags::kRound_Cap);
      flags.setStrokeJoin(PaintFlags::kRound_Join);
      flags.setStyle(PaintFlags::kFill_Style);
      flags.setTextEncoding(PaintFlags::kUTF32_TextEncoding);
      flags.setHinting(PaintFlags::kSlight_Hinting);
      flags.setFilterQuality(SkFilterQuality::kHigh_SkFilterQuality);

      SkScalar intervals[] = {1.f, 1.f};
      flags.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
      flags.setMaskFilter(
          SkBlurMaskFilter::Make(SkBlurStyle::kOuter_SkBlurStyle, 4.3f,
                                 test_rects[0], kHigh_SkBlurQuality));
      flags.setColorFilter(SkColorMatrixFilter::MakeLightingFilter(
          SK_ColorYELLOW, SK_ColorGREEN));

      SkLayerDrawLooper::Builder looper_builder;
      looper_builder.addLayer();
      looper_builder.addLayer(2.3f, 4.5f);
      SkLayerDrawLooper::LayerInfo layer_info;
      layer_info.fPaintBits |= SkLayerDrawLooper::kMaskFilter_Bit;
      layer_info.fColorMode = SkBlendMode::kDst;
      layer_info.fOffset.set(-1.f, 5.2f);
      looper_builder.addLayer(layer_info);
      flags.setLooper(looper_builder.detach());

      flags.setImageFilter(SkOffsetImageFilter::Make(10, 11, nullptr));

      sk_sp<PaintShader> shader = PaintShader::MakeColor(SK_ColorTRANSPARENT);
      PaintOpBufferTestUtils::FillArbitraryShaderValues(shader.get(), true);
      flags.setShader(std::move(shader));

      return flags;
    }(),
    [] {
      PaintFlags flags;
      flags.setShader(PaintShader::MakeColor(SkColorSetARGB(12, 34, 56, 78)));

      return flags;
    }(),
    [] {
      PaintFlags flags;
      sk_sp<PaintShader> shader = PaintShader::MakeColor(SK_ColorTRANSPARENT);
      PaintOpBufferTestUtils::FillArbitraryShaderValues(shader.get(), false);
      flags.setShader(std::move(shader));

      return flags;
    }(),
    [] {
      PaintFlags flags;
      SkPoint points[2] = {SkPoint::Make(1, 2), SkPoint::Make(3, 4)};
      SkColor colors[3] = {SkColorSetARGB(1, 2, 3, 4),
                           SkColorSetARGB(4, 3, 2, 1),
                           SkColorSetARGB(0, 10, 20, 30)};
      SkScalar positions[3] = {0.f, 0.3f, 1.f};
      flags.setShader(PaintShader::MakeLinearGradient(
          points, colors, positions, 3, SkShader::kMirror_TileMode));

      return flags;
    }(),
    PaintFlags(),
    PaintFlags(),
    PaintFlags(),
};

std::vector<SkColor> test_colors = {
    SkColorSetARGBInline(0, 0, 0, 0),
    SkColorSetARGBInline(255, 255, 255, 255),
    SkColorSetARGBInline(0, 255, 10, 255),
    SkColorSetARGBInline(255, 0, 20, 255),
    SkColorSetARGBInline(30, 255, 0, 255),
    SkColorSetARGBInline(255, 40, 0, 0),
};

std::vector<std::string> test_strings = {
    "", "foobar",
    "blarbideeblarasdfaiousydfp234poiausdofiuapsodfjknla;sdfkjasd;f",
};

std::vector<std::vector<SkPoint>> test_point_arrays = {
    std::vector<SkPoint>(),
    {SkPoint::Make(1, 2)},
    {SkPoint::Make(1, 2), SkPoint::Make(-5.4f, -3.8f)},
    {SkPoint::Make(0, 0), SkPoint::Make(5, 6), SkPoint::Make(-1, -1),
     SkPoint::Make(9, 9), SkPoint::Make(50, 50), SkPoint::Make(100, 100)},
};

std::vector<sk_sp<SkTextBlob>> test_blobs = {
    [] {
      SkPaint font;
      font.setTextEncoding(SkPaint::kGlyphID_TextEncoding);

      SkTextBlobBuilder builder;
      builder.allocRun(font, 5, 1.2f, 2.3f, &test_rects[0]);
      return builder.make();
    }(),
    [] {
      SkPaint font;
      font.setTextEncoding(SkPaint::kGlyphID_TextEncoding);

      SkTextBlobBuilder builder;
      builder.allocRun(font, 5, 1.2f, 2.3f, &test_rects[0]);
      builder.allocRunPos(font, 16, &test_rects[1]);
      builder.allocRunPosH(font, 8, 0, &test_rects[2]);
      return builder.make();
    }(),
};

// TODO(enne): In practice, probably all paint images need to be uploaded
// ahead of time and not be bitmaps. These paint images should be fake
// gpu resource paint images.
std::vector<PaintImage> test_images = {
    PaintImage(PaintImage::GetNextId(),
               CreateDiscardableImage(gfx::Size(5, 10))),
    PaintImage(PaintImage::GetNextId(),
               CreateDiscardableImage(gfx::Size(1, 1))),
    PaintImage(PaintImage::GetNextId(),
               CreateDiscardableImage(gfx::Size(50, 50))),
};

}  // namespace

const std::vector<float>& PaintOpBufferTestUtils::TestFloats() {
  return test_floats;
}
const std::vector<uint8_t>& PaintOpBufferTestUtils::TestUint8s() {
  return test_uint8s;
}
const std::vector<SkRect>& PaintOpBufferTestUtils::TestRects() {
  return test_rects;
}
const std::vector<SkRRect>& PaintOpBufferTestUtils::TestRRects() {
  return test_rrects;
}
const std::vector<SkIRect>& PaintOpBufferTestUtils::TestIRects() {
  return test_irects;
}
const std::vector<SkMatrix>& PaintOpBufferTestUtils::TestMatrices() {
  return test_matrices;
}
const std::vector<SkPath>& PaintOpBufferTestUtils::TestPaths() {
  return test_paths;
}
const std::vector<PaintFlags>& PaintOpBufferTestUtils::TestFlags() {
  return test_flags;
}
const std::vector<SkColor>& PaintOpBufferTestUtils::TestColors() {
  return test_colors;
}
const std::vector<std::string>& PaintOpBufferTestUtils::TestStrings() {
  return test_strings;
}
const std::vector<std::vector<SkPoint>>&
PaintOpBufferTestUtils::TestPointArrays() {
  return test_point_arrays;
}
const std::vector<sk_sp<SkTextBlob>>& PaintOpBufferTestUtils::TestBlobs() {
  return test_blobs;
}
const std::vector<PaintImage>& PaintOpBufferTestUtils::TestImages() {
  return test_images;
}

void PaintOpBufferTestUtils::PushAnnotateOps(PaintOpBuffer* buffer) {
  buffer->push<AnnotateOp>(PaintCanvas::AnnotationType::URL, test_rects[0],
                           SkData::MakeWithCString("thingerdoowhatchamagig"));
  // Deliberately test both null and empty SkData.
  buffer->push<AnnotateOp>(PaintCanvas::AnnotationType::LINK_TO_DESTINATION,
                           test_rects[1], nullptr);
  buffer->push<AnnotateOp>(PaintCanvas::AnnotationType::NAMED_DESTINATION,
                           test_rects[2], SkData::MakeEmpty());
}

void PaintOpBufferTestUtils::PushClipPathOps(PaintOpBuffer* buffer) {
  for (size_t i = 0; i < test_paths.size(); ++i) {
    SkClipOp op = i % 3 ? SkClipOp::kDifference : SkClipOp::kIntersect;
    buffer->push<ClipPathOp>(test_paths[i], op, !!(i % 2));
  }
}

void PaintOpBufferTestUtils::PushClipRectOps(PaintOpBuffer* buffer) {
  for (size_t i = 0; i < test_rects.size(); ++i) {
    SkClipOp op = i % 2 ? SkClipOp::kIntersect : SkClipOp::kDifference;
    bool antialias = !!(i % 3);
    buffer->push<ClipRectOp>(test_rects[i], op, antialias);
  }
}

void PaintOpBufferTestUtils::PushClipRRectOps(PaintOpBuffer* buffer) {
  for (size_t i = 0; i < test_rrects.size(); ++i) {
    SkClipOp op = i % 2 ? SkClipOp::kIntersect : SkClipOp::kDifference;
    bool antialias = !!(i % 3);
    buffer->push<ClipRRectOp>(test_rrects[i], op, antialias);
  }
}

void PaintOpBufferTestUtils::PushConcatOps(PaintOpBuffer* buffer) {
  for (size_t i = 0; i < test_matrices.size(); ++i)
    buffer->push<ConcatOp>(test_matrices[i]);
}

void PaintOpBufferTestUtils::PushDrawArcOps(PaintOpBuffer* buffer) {
  size_t len = std::min(std::min(test_floats.size() - 1, test_flags.size()),
                        test_rects.size());
  for (size_t i = 0; i < len; ++i) {
    bool use_center = !!(i % 2);
    buffer->push<DrawArcOp>(test_rects[i], test_floats[i], test_floats[i + 1],
                            use_center, test_flags[i]);
  }
}

void PaintOpBufferTestUtils::PushDrawCircleOps(PaintOpBuffer* buffer) {
  size_t len = std::min(test_floats.size() - 2, test_flags.size());
  for (size_t i = 0; i < len; ++i) {
    buffer->push<DrawCircleOp>(test_floats[i], test_floats[i + 1],
                               test_floats[i + 2], test_flags[i]);
  }
}

void PaintOpBufferTestUtils::PushDrawColorOps(PaintOpBuffer* buffer) {
  for (size_t i = 0; i < test_colors.size(); ++i) {
    buffer->push<DrawColorOp>(test_colors[i], static_cast<SkBlendMode>(i));
  }
}

void PaintOpBufferTestUtils::PushDrawDRRectOps(PaintOpBuffer* buffer) {
  size_t len = std::min(test_rrects.size() - 1, test_flags.size());
  for (size_t i = 0; i < len; ++i) {
    buffer->push<DrawDRRectOp>(test_rrects[i], test_rrects[i + 1],
                               test_flags[i]);
  }
}

void PaintOpBufferTestUtils::PushDrawImageOps(PaintOpBuffer* buffer) {
  size_t len = std::min(std::min(test_images.size(), test_flags.size()),
                        test_floats.size() - 1);
  for (size_t i = 0; i < len; ++i) {
    buffer->push<DrawImageOp>(test_images[i], test_floats[i],
                              test_floats[i + 1], &test_flags[i]);
  }

  // Test optional flags
  // TODO(enne): maybe all these optional ops should not be optional.
  buffer->push<DrawImageOp>(test_images[0], test_floats[0], test_floats[1],
                            nullptr);
}

void PaintOpBufferTestUtils::PushDrawImageRectOps(PaintOpBuffer* buffer) {
  size_t len = std::min(std::min(test_images.size(), test_flags.size()),
                        test_rects.size() - 1);
  for (size_t i = 0; i < len; ++i) {
    PaintCanvas::SrcRectConstraint constraint =
        i % 2 ? PaintCanvas::kStrict_SrcRectConstraint
              : PaintCanvas::kFast_SrcRectConstraint;
    buffer->push<DrawImageRectOp>(test_images[i], test_rects[i],
                                  test_rects[i + 1], &test_flags[i],
                                  constraint);
  }

  // Test optional flags.
  buffer->push<DrawImageRectOp>(test_images[0], test_rects[0], test_rects[1],
                                nullptr,
                                PaintCanvas::kStrict_SrcRectConstraint);
}

void PaintOpBufferTestUtils::PushDrawIRectOps(PaintOpBuffer* buffer) {
  size_t len = std::min(test_irects.size(), test_flags.size());
  for (size_t i = 0; i < len; ++i)
    buffer->push<DrawIRectOp>(test_irects[i], test_flags[i]);
}

void PaintOpBufferTestUtils::PushDrawLineOps(PaintOpBuffer* buffer) {
  size_t len = std::min(test_floats.size() - 3, test_flags.size());
  for (size_t i = 0; i < len; ++i) {
    buffer->push<DrawLineOp>(test_floats[i], test_floats[i + 1],
                             test_floats[i + 2], test_floats[i + 3],
                             test_flags[i]);
  }
}

void PaintOpBufferTestUtils::PushDrawOvalOps(PaintOpBuffer* buffer) {
  size_t len = std::min(test_paths.size(), test_flags.size());
  for (size_t i = 0; i < len; ++i)
    buffer->push<DrawOvalOp>(test_rects[i], test_flags[i]);
}

void PaintOpBufferTestUtils::PushDrawPathOps(PaintOpBuffer* buffer) {
  size_t len = std::min(test_paths.size(), test_flags.size());
  for (size_t i = 0; i < len; ++i)
    buffer->push<DrawPathOp>(test_paths[i], test_flags[i]);
}

void PaintOpBufferTestUtils::PushDrawPosTextOps(PaintOpBuffer* buffer) {
  size_t len = std::min(std::min(test_flags.size(), test_strings.size()),
                        test_point_arrays.size());
  for (size_t i = 0; i < len; ++i) {
    // Make sure empty array works fine.
    SkPoint* array =
        test_point_arrays[i].size() > 0 ? &test_point_arrays[i][0] : nullptr;
    buffer->push_with_array<DrawPosTextOp>(
        test_strings[i].c_str(), test_strings[i].size() + 1, array,
        test_point_arrays[i].size(), test_flags[i]);
  }
}

void PaintOpBufferTestUtils::PushDrawRectOps(PaintOpBuffer* buffer) {
  size_t len = std::min(test_rects.size(), test_flags.size());
  for (size_t i = 0; i < len; ++i)
    buffer->push<DrawRectOp>(test_rects[i], test_flags[i]);
}

void PaintOpBufferTestUtils::PushDrawRRectOps(PaintOpBuffer* buffer) {
  size_t len = std::min(test_rrects.size(), test_flags.size());
  for (size_t i = 0; i < len; ++i)
    buffer->push<DrawRRectOp>(test_rrects[i], test_flags[i]);
}

void PaintOpBufferTestUtils::PushDrawTextOps(PaintOpBuffer* buffer) {
  size_t len = std::min(std::min(test_strings.size(), test_flags.size()),
                        test_floats.size() - 1);
  for (size_t i = 0; i < len; ++i) {
    buffer->push_with_data<DrawTextOp>(
        test_strings[i].c_str(), test_strings[i].size() + 1, test_floats[i],
        test_floats[i + 1], test_flags[i]);
  }
}

void PaintOpBufferTestUtils::PushDrawTextBlobOps(PaintOpBuffer* buffer) {
  size_t len = std::min(std::min(test_blobs.size(), test_flags.size()),
                        test_floats.size() - 1);
  for (size_t i = 0; i < len; ++i) {
    buffer->push<DrawTextBlobOp>(test_blobs[i], test_floats[i],
                                 test_floats[i + 1], test_flags[i]);
  }
}

void PaintOpBufferTestUtils::PushNoopOps(PaintOpBuffer* buffer) {
  for (int i = 0; i < 4; ++i)
    buffer->push<NoopOp>();
}

void PaintOpBufferTestUtils::PushRestoreOps(PaintOpBuffer* buffer) {
  for (int i = 0; i < 4; ++i)
    buffer->push<RestoreOp>();
}

void PaintOpBufferTestUtils::PushRotateOps(PaintOpBuffer* buffer) {
  for (size_t i = 0; i < test_floats.size(); ++i)
    buffer->push<RotateOp>(test_floats[i]);
}

void PaintOpBufferTestUtils::PushSaveOps(PaintOpBuffer* buffer) {
  for (int i = 0; i < 4; ++i)
    buffer->push<SaveOp>();
}

void PaintOpBufferTestUtils::PushSaveLayerOps(PaintOpBuffer* buffer) {
  size_t len = std::min(test_flags.size(), test_rects.size());
  for (size_t i = 0; i < len; ++i)
    buffer->push<SaveLayerOp>(&test_rects[i], &test_flags[i]);

  // Test combinations of optional args.
  buffer->push<SaveLayerOp>(nullptr, &test_flags[0]);
  buffer->push<SaveLayerOp>(&test_rects[0], nullptr);
  buffer->push<SaveLayerOp>(nullptr, nullptr);
}

void PaintOpBufferTestUtils::PushSaveLayerAlphaOps(PaintOpBuffer* buffer) {
  size_t len = std::min(test_uint8s.size(), test_rects.size());
  for (size_t i = 0; i < len; ++i)
    buffer->push<SaveLayerAlphaOp>(&test_rects[i], test_uint8s[i], !!(i % 2));

  // Test optional args.
  buffer->push<SaveLayerAlphaOp>(nullptr, test_uint8s[0], false);
}

void PaintOpBufferTestUtils::PushScaleOps(PaintOpBuffer* buffer) {
  for (size_t i = 0; i < test_floats.size(); i += 2)
    buffer->push<ScaleOp>(test_floats[i], test_floats[i + 1]);
}

void PaintOpBufferTestUtils::PushSetMatrixOps(PaintOpBuffer* buffer) {
  for (size_t i = 0; i < test_matrices.size(); ++i)
    buffer->push<SetMatrixOp>(test_matrices[i]);
}

void PaintOpBufferTestUtils::PushTranslateOps(PaintOpBuffer* buffer) {
  for (size_t i = 0; i < test_floats.size(); i += 2)
    buffer->push<TranslateOp>(test_floats[i], test_floats[i + 1]);
}

void PaintOpBufferTestUtils::ExpectOpsEqual(const PaintOp* one,
                                            const PaintOp* two) {
  ASSERT_TRUE(one);
  ASSERT_TRUE(two);
  ASSERT_EQ(one->GetType(), two->GetType());
  EXPECT_EQ(one->skip, two->skip);

  switch (one->GetType()) {
    case PaintOpType::Annotate:
      ExpectEqual(static_cast<const AnnotateOp*>(one),
                  static_cast<const AnnotateOp*>(two));
      break;
    case PaintOpType::ClipPath:
      ExpectEqual(static_cast<const ClipPathOp*>(one),
                  static_cast<const ClipPathOp*>(two));
      break;
    case PaintOpType::ClipRect:
      ExpectEqual(static_cast<const ClipRectOp*>(one),
                  static_cast<const ClipRectOp*>(two));
      break;
    case PaintOpType::ClipRRect:
      ExpectEqual(static_cast<const ClipRRectOp*>(one),
                  static_cast<const ClipRRectOp*>(two));
      break;
    case PaintOpType::Concat:
      ExpectEqual(static_cast<const ConcatOp*>(one),
                  static_cast<const ConcatOp*>(two));
      break;
    case PaintOpType::DrawArc:
      ExpectEqual(static_cast<const DrawArcOp*>(one),
                  static_cast<const DrawArcOp*>(two));
      break;
    case PaintOpType::DrawCircle:
      ExpectEqual(static_cast<const DrawCircleOp*>(one),
                  static_cast<const DrawCircleOp*>(two));
      break;
    case PaintOpType::DrawColor:
      ExpectEqual(static_cast<const DrawColorOp*>(one),
                  static_cast<const DrawColorOp*>(two));
      break;
    case PaintOpType::DrawDRRect:
      ExpectEqual(static_cast<const DrawDRRectOp*>(one),
                  static_cast<const DrawDRRectOp*>(two));
      break;
    case PaintOpType::DrawImage:
      ExpectEqual(static_cast<const DrawImageOp*>(one),
                  static_cast<const DrawImageOp*>(two));
      break;
    case PaintOpType::DrawImageRect:
      ExpectEqual(static_cast<const DrawImageRectOp*>(one),
                  static_cast<const DrawImageRectOp*>(two));
      break;
    case PaintOpType::DrawIRect:
      ExpectEqual(static_cast<const DrawIRectOp*>(one),
                  static_cast<const DrawIRectOp*>(two));
      break;
    case PaintOpType::DrawLine:
      ExpectEqual(static_cast<const DrawLineOp*>(one),
                  static_cast<const DrawLineOp*>(two));
      break;
    case PaintOpType::DrawOval:
      ExpectEqual(static_cast<const DrawOvalOp*>(one),
                  static_cast<const DrawOvalOp*>(two));
      break;
    case PaintOpType::DrawPath:
      ExpectEqual(static_cast<const DrawPathOp*>(one),
                  static_cast<const DrawPathOp*>(two));
      break;
    case PaintOpType::DrawPosText:
      ExpectEqual(static_cast<const DrawPosTextOp*>(one),
                  static_cast<const DrawPosTextOp*>(two));
      break;
    case PaintOpType::DrawRecord:
      // Not supported.
      break;
    case PaintOpType::DrawRect:
      ExpectEqual(static_cast<const DrawRectOp*>(one),
                  static_cast<const DrawRectOp*>(two));
      break;
    case PaintOpType::DrawRRect:
      ExpectEqual(static_cast<const DrawRRectOp*>(one),
                  static_cast<const DrawRRectOp*>(two));
      break;
    case PaintOpType::DrawText:
      ExpectEqual(static_cast<const DrawTextOp*>(one),
                  static_cast<const DrawTextOp*>(two));
      break;
    case PaintOpType::DrawTextBlob:
      ExpectEqual(static_cast<const DrawTextBlobOp*>(one),
                  static_cast<const DrawTextBlobOp*>(two));
      break;
    case PaintOpType::Noop:
      ExpectEqual(static_cast<const NoopOp*>(one),
                  static_cast<const NoopOp*>(two));
      break;
    case PaintOpType::Restore:
      ExpectEqual(static_cast<const RestoreOp*>(one),
                  static_cast<const RestoreOp*>(two));
      break;
    case PaintOpType::Rotate:
      ExpectEqual(static_cast<const RotateOp*>(one),
                  static_cast<const RotateOp*>(two));
      break;
    case PaintOpType::Save:
      ExpectEqual(static_cast<const SaveOp*>(one),
                  static_cast<const SaveOp*>(two));
      break;
    case PaintOpType::SaveLayer:
      ExpectEqual(static_cast<const SaveLayerOp*>(one),
                  static_cast<const SaveLayerOp*>(two));
      break;
    case PaintOpType::SaveLayerAlpha:
      ExpectEqual(static_cast<const SaveLayerAlphaOp*>(one),
                  static_cast<const SaveLayerAlphaOp*>(two));
      break;
    case PaintOpType::Scale:
      ExpectEqual(static_cast<const ScaleOp*>(one),
                  static_cast<const ScaleOp*>(two));
      break;
    case PaintOpType::SetMatrix:
      ExpectEqual(static_cast<const SetMatrixOp*>(one),
                  static_cast<const SetMatrixOp*>(two));
      break;
    case PaintOpType::Translate:
      ExpectEqual(static_cast<const TranslateOp*>(one),
                  static_cast<const TranslateOp*>(two));
      break;
  }
}

void PaintOpBufferTestUtils::ExpectEqual(SkFlattenable* expected,
                                         SkFlattenable* actual) {
  sk_sp<SkData> expected_data(SkValidatingSerializeFlattenable(expected));
  sk_sp<SkData> actual_data(SkValidatingSerializeFlattenable(actual));
  ASSERT_EQ(expected_data->size(), actual_data->size());
  EXPECT_TRUE(expected_data->equals(actual_data.get()));
}

void PaintOpBufferTestUtils::ExpectEqual(const PaintShader* one,
                                         const PaintShader* two) {
  if (!one) {
    EXPECT_FALSE(two);
    return;
  }

  ASSERT_TRUE(one);
  ASSERT_TRUE(two);

  EXPECT_EQ(one->shader_type_, two->shader_type_);
  EXPECT_EQ(one->flags_, two->flags_);
  EXPECT_EQ(one->end_radius_, two->end_radius_);
  EXPECT_EQ(one->start_radius_, two->start_radius_);
  EXPECT_EQ(one->tx_, two->tx_);
  EXPECT_EQ(one->ty_, two->ty_);
  EXPECT_EQ(one->fallback_color_, two->fallback_color_);
  EXPECT_EQ(one->scaling_behavior_, two->scaling_behavior_);
  if (one->local_matrix_) {
    EXPECT_TRUE(two->local_matrix_.has_value());
    EXPECT_TRUE(*one->local_matrix_ == *two->local_matrix_);
  } else {
    EXPECT_FALSE(two->local_matrix_.has_value());
  }
  EXPECT_EQ(one->center_, two->center_);
  EXPECT_EQ(one->tile_, two->tile_);
  EXPECT_EQ(one->start_point_, two->start_point_);
  EXPECT_EQ(one->end_point_, two->end_point_);
  EXPECT_THAT(one->colors_, testing::ElementsAreArray(two->colors_));
  EXPECT_THAT(one->positions_, testing::ElementsAreArray(two->positions_));
}

void PaintOpBufferTestUtils::ExpectEqual(const PaintFlags& expected,
                                         const PaintFlags& actual) {
  // Can't just ToSkPaint and operator== here as SkPaint does pointer
  // comparisons on all the ref'd skia objects on the SkPaint, which
  // is not true after serialization.
  EXPECT_EQ(expected.getTextSize(), actual.getTextSize());
  EXPECT_EQ(expected.getColor(), actual.getColor());
  EXPECT_EQ(expected.getStrokeWidth(), actual.getStrokeWidth());
  EXPECT_EQ(expected.getStrokeMiter(), actual.getStrokeMiter());
  EXPECT_EQ(expected.getBlendMode(), actual.getBlendMode());
  EXPECT_EQ(expected.getStrokeCap(), actual.getStrokeCap());
  EXPECT_EQ(expected.getStrokeJoin(), actual.getStrokeJoin());
  EXPECT_EQ(expected.getStyle(), actual.getStyle());
  EXPECT_EQ(expected.getTextEncoding(), actual.getTextEncoding());
  EXPECT_EQ(expected.getHinting(), actual.getHinting());
  EXPECT_EQ(expected.getFilterQuality(), actual.getFilterQuality());

  // TODO(enne): compare typeface too

  // Flattenable comparisons.
  ExpectEqual(expected.getPathEffect().get(), actual.getPathEffect().get());
  ExpectEqual(expected.getMaskFilter().get(), actual.getMaskFilter().get());
  ExpectEqual(expected.getColorFilter().get(), actual.getColorFilter().get());
  ExpectEqual(expected.getLooper().get(), actual.getLooper().get());
  ExpectEqual(expected.getImageFilter().get(), actual.getImageFilter().get());

  ExpectEqual(expected.getShader(), actual.getShader());
}

void PaintOpBufferTestUtils::ExpectEqual(const PaintImage& original,
                                         const PaintImage& written) {}

void PaintOpBufferTestUtils::ExpectEqual(const AnnotateOp* original,
                                         const AnnotateOp* written) {
  EXPECT_EQ(original->annotation_type, written->annotation_type);
  EXPECT_EQ(original->rect, written->rect);
  EXPECT_EQ(!!original->data, !!written->data);
  if (original->data) {
    EXPECT_EQ(original->data->size(), written->data->size());
    EXPECT_EQ(0, memcmp(original->data->data(), written->data->data(),
                        written->data->size()));
  }
}

void PaintOpBufferTestUtils::ExpectEqual(const ClipPathOp* original,
                                         const ClipPathOp* written) {
  EXPECT_TRUE(original->path == written->path);
  EXPECT_EQ(original->op, written->op);
  EXPECT_EQ(original->antialias, written->antialias);
}

void PaintOpBufferTestUtils::ExpectEqual(const ClipRectOp* original,
                                         const ClipRectOp* written) {
  EXPECT_EQ(original->rect, written->rect);
  EXPECT_EQ(original->op, written->op);
  EXPECT_EQ(original->antialias, written->antialias);
}

void PaintOpBufferTestUtils::ExpectEqual(const ClipRRectOp* original,
                                         const ClipRRectOp* written) {
  EXPECT_EQ(original->rrect, written->rrect);
  EXPECT_EQ(original->op, written->op);
  EXPECT_EQ(original->antialias, written->antialias);
}

void PaintOpBufferTestUtils::ExpectEqual(const ConcatOp* original,
                                         const ConcatOp* written) {
  EXPECT_EQ(original->matrix, written->matrix);
  EXPECT_EQ(original->matrix.getType(), written->matrix.getType());
}

void PaintOpBufferTestUtils::ExpectEqual(const DrawArcOp* original,
                                         const DrawArcOp* written) {
  ExpectEqual(original->flags, written->flags);
  EXPECT_EQ(original->oval, written->oval);
  EXPECT_EQ(original->start_angle, written->start_angle);
  EXPECT_EQ(original->sweep_angle, written->sweep_angle);
  EXPECT_EQ(original->use_center, written->use_center);
}

void PaintOpBufferTestUtils::ExpectEqual(const DrawCircleOp* original,
                                         const DrawCircleOp* written) {
  ExpectEqual(original->flags, written->flags);
  EXPECT_EQ(original->cx, written->cx);
  EXPECT_EQ(original->cy, written->cy);
  EXPECT_EQ(original->radius, written->radius);
}

void PaintOpBufferTestUtils::ExpectEqual(const DrawColorOp* original,
                                         const DrawColorOp* written) {
  EXPECT_EQ(original->color, written->color);
}

void PaintOpBufferTestUtils::ExpectEqual(const DrawDRRectOp* original,
                                         const DrawDRRectOp* written) {
  ExpectEqual(original->flags, written->flags);
  EXPECT_EQ(original->outer, written->outer);
  EXPECT_EQ(original->inner, written->inner);
}

void PaintOpBufferTestUtils::ExpectEqual(const DrawImageOp* original,
                                         const DrawImageOp* written) {
  ExpectEqual(original->flags, written->flags);
  ExpectEqual(original->image, written->image);
  EXPECT_EQ(original->left, written->left);
  EXPECT_EQ(original->top, written->top);
}

void PaintOpBufferTestUtils::ExpectEqual(const DrawImageRectOp* original,
                                         const DrawImageRectOp* written) {
  ExpectEqual(original->flags, written->flags);
  ExpectEqual(original->image, written->image);
  EXPECT_EQ(original->src, written->src);
  EXPECT_EQ(original->dst, written->dst);
}

void PaintOpBufferTestUtils::ExpectEqual(const DrawIRectOp* original,
                                         const DrawIRectOp* written) {
  ExpectEqual(original->flags, written->flags);
  EXPECT_EQ(original->rect, written->rect);
}

void PaintOpBufferTestUtils::ExpectEqual(const DrawLineOp* original,
                                         const DrawLineOp* written) {
  ExpectEqual(original->flags, written->flags);
  EXPECT_EQ(original->x0, written->x0);
  EXPECT_EQ(original->y0, written->y0);
  EXPECT_EQ(original->x1, written->x1);
  EXPECT_EQ(original->y1, written->y1);
}

void PaintOpBufferTestUtils::ExpectEqual(const DrawOvalOp* original,
                                         const DrawOvalOp* written) {
  ExpectEqual(original->flags, written->flags);
  EXPECT_EQ(original->oval, written->oval);
}

void PaintOpBufferTestUtils::ExpectEqual(const DrawPathOp* original,
                                         const DrawPathOp* written) {
  ExpectEqual(original->flags, written->flags);
  EXPECT_TRUE(original->path == written->path);
}

void PaintOpBufferTestUtils::ExpectEqual(const DrawPosTextOp* original,
                                         const DrawPosTextOp* written) {
  ExpectEqual(original->flags, written->flags);
  ASSERT_EQ(original->bytes, written->bytes);
  EXPECT_EQ(std::string(static_cast<const char*>(original->GetData())),
            std::string(static_cast<const char*>(written->GetData())));
  ASSERT_EQ(original->count, written->count);
  for (size_t i = 0; i < original->count; ++i)
    EXPECT_EQ(original->GetArray()[i], written->GetArray()[i]);
}

void PaintOpBufferTestUtils::ExpectEqual(const DrawRectOp* original,
                                         const DrawRectOp* written) {
  ExpectEqual(original->flags, written->flags);
  EXPECT_EQ(original->rect, written->rect);
}

void PaintOpBufferTestUtils::ExpectEqual(const DrawRRectOp* original,
                                         const DrawRRectOp* written) {
  ExpectEqual(original->flags, written->flags);
  EXPECT_EQ(original->rrect, written->rrect);
}

void PaintOpBufferTestUtils::ExpectEqual(const DrawTextOp* original,
                                         const DrawTextOp* written) {
  ExpectEqual(original->flags, written->flags);
  EXPECT_EQ(original->x, written->x);
  EXPECT_EQ(original->y, written->y);
  ASSERT_EQ(original->bytes, written->bytes);
  EXPECT_EQ(std::string(static_cast<const char*>(original->GetData())),
            std::string(static_cast<const char*>(written->GetData())));
}

void PaintOpBufferTestUtils::ExpectEqual(const DrawTextBlobOp* original,
                                         const DrawTextBlobOp* written) {
  ExpectEqual(original->flags, written->flags);
  EXPECT_EQ(original->x, written->x);
  EXPECT_EQ(original->y, written->y);

  // TODO(enne): implement SkTextBlob serialization: http://crbug.com/737629
  if (!original->blob || !written->blob)
    return;

  ASSERT_TRUE(original->blob);
  ASSERT_TRUE(written->blob);

  // No text blob operator==, so flatten them both and compare.
  size_t max_size = original->skip;

  std::vector<char> original_mem;
  original_mem.resize(max_size);
  SkBinaryWriteBuffer original_flattened(&original_mem[0], max_size);
  original->blob->flatten(original_flattened);
  original_mem.resize(original_flattened.bytesWritten());

  std::vector<char> written_mem;
  written_mem.resize(max_size);
  SkBinaryWriteBuffer written_flattened(&written_mem[0], max_size);
  written->blob->flatten(written_flattened);
  written_mem.resize(written_flattened.bytesWritten());

  ASSERT_EQ(original_mem.size(), written_mem.size());
  EXPECT_EQ(original_mem, written_mem);
}

void PaintOpBufferTestUtils::ExpectEqual(const NoopOp* original,
                                         const NoopOp* written) {
  // Nothing to compare.
}

void PaintOpBufferTestUtils::ExpectEqual(const RestoreOp* original,
                                         const RestoreOp* written) {
  // Nothing to compare.
}

void PaintOpBufferTestUtils::ExpectEqual(const RotateOp* original,
                                         const RotateOp* written) {
  EXPECT_EQ(original->degrees, written->degrees);
}

void PaintOpBufferTestUtils::ExpectEqual(const SaveOp* original,
                                         const SaveOp* written) {
  // Nothing to compare.
}

void PaintOpBufferTestUtils::ExpectEqual(const SaveLayerOp* original,
                                         const SaveLayerOp* written) {
  ExpectEqual(original->flags, written->flags);
  EXPECT_EQ(original->bounds, written->bounds);
}

void PaintOpBufferTestUtils::ExpectEqual(const SaveLayerAlphaOp* original,
                                         const SaveLayerAlphaOp* written) {
  EXPECT_EQ(original->bounds, written->bounds);
  EXPECT_EQ(original->alpha, written->alpha);
  EXPECT_EQ(original->preserve_lcd_text_requests,
            written->preserve_lcd_text_requests);
}

void PaintOpBufferTestUtils::ExpectEqual(const ScaleOp* original,
                                         const ScaleOp* written) {
  EXPECT_EQ(original->sx, written->sx);
  EXPECT_EQ(original->sy, written->sy);
}

void PaintOpBufferTestUtils::ExpectEqual(const SetMatrixOp* original,
                                         const SetMatrixOp* written) {
  EXPECT_EQ(original->matrix, written->matrix);
}

void PaintOpBufferTestUtils::ExpectEqual(const TranslateOp* original,
                                         const TranslateOp* written) {
  EXPECT_EQ(original->dx, written->dx);
  EXPECT_EQ(original->dy, written->dy);
}

void PaintOpBufferTestUtils::FillArbitraryShaderValues(PaintShader* shader,
                                                       bool use_matrix) {
  shader->shader_type_ = PaintShader::Type::kTwoPointConicalGradient;
  shader->flags_ = 12345;
  shader->end_radius_ = 12.3f;
  shader->start_radius_ = 13.4f;
  shader->tx_ = SkShader::kRepeat_TileMode;
  shader->ty_ = SkShader::kMirror_TileMode;
  shader->fallback_color_ = SkColorSetARGB(254, 252, 250, 248);
  shader->scaling_behavior_ = PaintShader::ScalingBehavior::kRasterAtScale;
  if (use_matrix) {
    shader->local_matrix_.emplace(SkMatrix::I());
    shader->local_matrix_->setSkewX(10);
    shader->local_matrix_->setSkewY(20);
  }
  shader->center_ = SkPoint::Make(50, 40);
  shader->tile_ = SkRect::MakeXYWH(7, 77, 777, 7777);
  shader->start_point_ = SkPoint::Make(-1, -5);
  shader->end_point_ = SkPoint::Make(13, -13);
  // TODO(vmpstr): Add PaintImage/PaintRecord.
  shader->colors_ = {SkColorSetARGB(1, 2, 3, 4), SkColorSetARGB(5, 6, 7, 8),
                     SkColorSetARGB(9, 0, 1, 2)};
  shader->positions_ = {0.f, 0.4f, 1.f};
}

}  // namespace cc

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/resolver/FontStyleResolver.h"
#include "core/css/threaded/MultiThreadedTestUtil.h"
#include "platform/Language.h"
#include "platform/fonts/Font.h"
#include "platform/fonts/FontCustomPlatformData.h"
#include "platform/fonts/FontDescription.h"
#include "platform/fonts/FontSelector.h"
#include "platform/fonts/shaping/CachingWordShapeIterator.h"
#include "platform/fonts/shaping/HarfBuzzShaper.h"
#include "platform/graphics/paint/PaintCanvas.h"
#include "platform/testing/FontTestHelpers.h"
#include "platform/testing/UnitTestHelpers.h"
#include "platform/text/TextDirection.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkMetaData.h"

using ::testing::_;
using ::testing::Return;

using blink::testing::CreateTestFont;

namespace blink {

class MockPaintCanvas : public PaintCanvas {
 public:
  MOCK_METHOD0(getMetaData, SkMetaData&());
  MOCK_CONST_METHOD0(imageInfo, SkImageInfo());
  MOCK_METHOD0(flush, void());
  MOCK_METHOD0(save, int());
  MOCK_METHOD2(saveLayer, int(const SkRect* bounds, const PaintFlags* flags));
  MOCK_METHOD3(saveLayerAlpha,
               int(const SkRect* bounds,
                   uint8_t alpha,
                   bool preserve_lcd_text_requests));
  MOCK_METHOD0(restore, void());
  MOCK_CONST_METHOD0(getSaveCount, int());
  MOCK_METHOD1(restoreToCount, void(int save_count));
  MOCK_METHOD2(translate, void(SkScalar dx, SkScalar dy));
  MOCK_METHOD2(scale, void(SkScalar sx, SkScalar sy));
  MOCK_METHOD1(rotate, void(SkScalar degrees));
  MOCK_METHOD1(concat, void(const SkMatrix& matrix));
  MOCK_METHOD1(setMatrix, void(const SkMatrix& matrix));
  MOCK_METHOD3(clipRect,
               void(const SkRect& rect, SkClipOp op, bool do_anti_alias));
  MOCK_METHOD3(clipRRect,
               void(const SkRRect& rrect, SkClipOp op, bool do_anti_alias));
  MOCK_METHOD3(clipPath,
               void(const SkPath& path, SkClipOp op, bool do_anti_alias));
  MOCK_CONST_METHOD1(quickReject, bool(const SkRect& rect));
  MOCK_CONST_METHOD1(quickReject, bool(const SkPath& path));
  MOCK_CONST_METHOD0(getLocalClipBounds, SkRect());
  MOCK_CONST_METHOD1(getLocalClipBounds, bool(SkRect* bounds));
  MOCK_CONST_METHOD0(getDeviceClipBounds, SkIRect());
  MOCK_CONST_METHOD1(getDeviceClipBounds, bool(SkIRect* bounds));
  MOCK_METHOD2(drawColor, void(SkColor color, SkBlendMode mode));
  MOCK_METHOD1(clear, void(SkColor color));
  MOCK_METHOD5(drawLine,
               void(SkScalar x0,
                    SkScalar y0,
                    SkScalar x1,
                    SkScalar y1,
                    const PaintFlags& flags));
  MOCK_METHOD2(drawRect, void(const SkRect& rect, const PaintFlags& flags));
  MOCK_METHOD2(drawIRect, void(const SkIRect& rect, const PaintFlags& flags));
  MOCK_METHOD2(drawOval, void(const SkRect& oval, const PaintFlags& flags));
  MOCK_METHOD2(drawRRect, void(const SkRRect& rrect, const PaintFlags& flags));
  MOCK_METHOD3(drawDRRect,
               void(const SkRRect& outer,
                    const SkRRect& inner,
                    const PaintFlags& flags));
  MOCK_METHOD4(
      drawCircle,
      void(SkScalar cx, SkScalar cy, SkScalar radius, const PaintFlags& flags));
  MOCK_METHOD5(drawArc,
               void(const SkRect& oval,
                    SkScalar start_angle,
                    SkScalar sweep_angle,
                    bool use_center,
                    const PaintFlags& flags));
  MOCK_METHOD4(drawRoundRect,
               void(const SkRect& rect,
                    SkScalar rx,
                    SkScalar ry,
                    const PaintFlags& flags));
  MOCK_METHOD2(drawPath, void(const SkPath& path, const PaintFlags& flags));
  MOCK_METHOD4(drawImage,
               void(const PaintImage& image,
                    SkScalar left,
                    SkScalar top,
                    const PaintFlags* flags));
  MOCK_METHOD5(drawImageRect,
               void(const PaintImage& image,
                    const SkRect& src,
                    const SkRect& dst,
                    const PaintFlags* flags,
                    SrcRectConstraint constraint));
  MOCK_METHOD4(drawBitmap,
               void(const SkBitmap& bitmap,
                    SkScalar left,
                    SkScalar top,
                    const PaintFlags* flags));
  MOCK_METHOD5(drawText,
               void(const void* text,
                    size_t byte_length,
                    SkScalar x,
                    SkScalar y,
                    const PaintFlags& flags));
  MOCK_METHOD4(drawPosText,
               void(const void* text,
                    size_t byte_length,
                    const SkPoint pos[],
                    const PaintFlags& flags));
  MOCK_METHOD4(drawTextBlob,
               void(sk_sp<SkTextBlob> blob,
                    SkScalar x,
                    SkScalar y,
                    const PaintFlags& flags));
  MOCK_METHOD1(drawPicture, void(sk_sp<const PaintRecord> record));
  MOCK_CONST_METHOD0(isClipEmpty, bool());
  MOCK_CONST_METHOD0(isClipRect, bool());
  MOCK_CONST_METHOD0(getTotalMatrix, const SkMatrix&());

  MOCK_METHOD3(Annotate,
               void(AnnotationType type,
                    const SkRect& rect,
                    sk_sp<SkData> data));
};

TSAN_TEST(TextRendererThreadedTest, measureText) {
  // callbacks_per_thread_ = 20;
  RunOnThreads([]() {
    String text = "measure this";

    FontDescription font_description;
    font_description.SetComputedSize(12.0);
    font_description.SetLocale(LayoutLocale::Get("en"));
    ASSERT_EQ(USCRIPT_LATIN, font_description.GetScript());
    font_description.SetGenericFamily(FontDescription::kStandardFamily);

    Font font = Font(font_description);
    font.Update(nullptr);

    const SimpleFontData* font_data = font.PrimaryFont();
    ASSERT_TRUE(font_data);

    TextRun text_run(
        text, 0, 0,
        TextRun::kAllowTrailingExpansion | TextRun::kForbidLeadingExpansion,
        TextDirection::kLtr, false);
    text_run.SetNormalizeSpace(true);
    FloatRect text_bounds = font.SelectionRectForText(
        text_run, FloatPoint(), font.GetFontDescription().ComputedSize(), 0, -1,
        true);

    // x direction
    EXPECT_FLOAT_EQ(77.7363, font.Width(text_run));
    EXPECT_EQ(0, text_bounds.X());
    EXPECT_EQ(78, text_bounds.MaxX());

    // y direction
    const FontMetrics& font_metrics = font_data->GetFontMetrics();
    EXPECT_EQ(11, font_metrics.FloatAscent());
    EXPECT_EQ(3, font_metrics.FloatDescent());
    EXPECT_EQ(0, text_bounds.Y());
    EXPECT_EQ(12, text_bounds.MaxY());
  });
}

TSAN_TEST(TextRendererThreadedTest, drawText) {
  callbacks_per_thread_ = 50;
  RunOnThreads([]() {
    String text = "draw this";

    FontDescription font_description;
    font_description.SetComputedSize(12.0);
    font_description.SetLocale(LayoutLocale::Get("en"));
    ASSERT_EQ(USCRIPT_LATIN, font_description.GetScript());
    font_description.SetGenericFamily(FontDescription::kStandardFamily);

    Font font = Font(font_description);
    font.Update(nullptr);

    const SimpleFontData* font_data = font.PrimaryFont();
    ASSERT_TRUE(font_data);

    const FontMetrics& font_metrics = font_data->GetFontMetrics();

    FloatPoint location(0, 0);
    TextRun text_run(text, 0, 0, TextRun::kAllowTrailingExpansion,
                     TextDirection::kLtr, false);
    text_run.SetNormalizeSpace(true);
    float width = font.Width(text_run);

    TextRunPaintInfo text_run_paint_info(text_run);

    text_run_paint_info.bounds =
        FloatRect(location.X() - font_metrics.Height() / 2,
                  location.Y() - font_metrics.Ascent() - font_metrics.LineGap(),
                  width + font_metrics.Height(), font_metrics.LineSpacing());

    MockPaintCanvas mpc;
    PaintFlags flags;

    EXPECT_CALL(mpc, getSaveCount()).WillOnce(Return(17));
    EXPECT_CALL(mpc, drawTextBlob(_, 0, 0, _)).Times(1);
    EXPECT_CALL(mpc, restoreToCount(17)).WillOnce(Return());

    font.DrawBidiText(&mpc, text_run_paint_info, location,
                      Font::kUseFallbackIfFontNotReady, 1.0, flags);
  });
}

}  // namespace blink

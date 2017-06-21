// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/solid_color_analyzer.h"
#include "base/optional.h"
#include "cc/paint/record_paint_canvas.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/effects/SkOffsetImageFilter.h"
#include "ui/gfx/skia_util.h"

namespace cc {
namespace {

class SolidColorAnalyzerTest : public testing::Test {
 public:
  void SetUp() override {}

  void TearDown() override {
    analyzer_.reset();
    canvas_.reset();
    buffer_.Reset();
    analysis_ran_ = false;
  }

  void Initialize(const gfx::Rect& rect = gfx::Rect(0, 0, 100, 100)) {
    canvas_.emplace(&buffer_, gfx::RectToSkRect(rect));
    analyzer_.emplace(&buffer_);
  }
  RecordPaintCanvas* canvas() { return &*canvas_; }
  PaintOpBuffer* paint_op_buffer() { return &buffer_; }

  bool IsSolidColor() const {
    EXPECT_TRUE(analysis_ran_) << "Analysis not run.";
    SkColor color = SK_ColorBLACK;
    return analyzer_->GetColorIfSolid(&color);
  }
  SkColor GetColor() const {
    EXPECT_TRUE(analysis_ran_) << "Analysis not run.";
    SkColor color = SK_ColorBLACK;
    EXPECT_TRUE(analyzer_->GetColorIfSolid(&color));
    return color;
  }

  void RunAnalysis(const gfx::Rect& rect) {
    EXPECT_FALSE(analysis_ran_) << "Analysis already ran.";
    analysis_ran_ = true;
    analyzer_->RunAnalysis(rect, nullptr);
  }
  void RunAnalysis(const gfx::Rect& rect, const std::vector<size_t>& indices) {
    EXPECT_FALSE(analysis_ran_) << "Analysis already ran.";
    analysis_ran_ = true;
    analyzer_->RunAnalysis(rect, &indices);
  }

 private:
  PaintOpBuffer buffer_;
  base::Optional<RecordPaintCanvas> canvas_;
  base::Optional<SolidColorAnalyzer> analyzer_;
  bool analysis_ran_ = false;
};

TEST_F(SolidColorAnalyzerTest, Empty) {
  Initialize();
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_EQ(SK_ColorTRANSPARENT, GetColor());
}

TEST_F(SolidColorAnalyzerTest, ClearTransparent) {
  Initialize();
  SkColor color = SkColorSetARGB(0, 12, 34, 56);
  canvas()->clear(color);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_EQ(SK_ColorTRANSPARENT, GetColor());
}

TEST_F(SolidColorAnalyzerTest, ClearSolid) {
  Initialize();
  SkColor color = SkColorSetARGB(255, 65, 43, 21);
  canvas()->clear(color);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_EQ(color, GetColor());
}

TEST_F(SolidColorAnalyzerTest, ClearTranslucent) {
  Initialize();
  SkColor color = SkColorSetARGB(128, 11, 22, 33);
  canvas()->clear(color);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_FALSE(IsSolidColor());
}

TEST_F(SolidColorAnalyzerTest, DrawColor) {
  Initialize();
  SkColor color = SkColorSetARGB(255, 11, 22, 33);
  canvas()->drawColor(color);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_EQ(color, GetColor());
}

TEST_F(SolidColorAnalyzerTest, DrawOval) {
  Initialize();
  PaintFlags flags;
  SkColor color = SkColorSetARGB(255, 11, 22, 33);
  flags.setColor(color);
  canvas()->drawOval(SkRect::MakeWH(100, 100), flags);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_FALSE(IsSolidColor());
}

TEST_F(SolidColorAnalyzerTest, DrawBitmap) {
  Initialize();
  SkBitmap bitmap;
  bitmap.allocN32Pixels(16, 16);
  canvas()->drawBitmap(bitmap, 0, 0, nullptr);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_FALSE(IsSolidColor());
}

TEST_F(SolidColorAnalyzerTest, DrawRect) {
  Initialize();
  PaintFlags flags;
  SkColor color = SkColorSetARGB(255, 11, 22, 33);
  flags.setColor(color);
  SkRect rect = SkRect::MakeWH(200, 200);
  canvas()->clipRect(rect, SkClipOp::kIntersect, false);
  canvas()->drawRect(rect, flags);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_EQ(color, GetColor());
}

TEST_F(SolidColorAnalyzerTest, DrawRectWithTranslateNotSolid) {
  Initialize();
  PaintFlags flags;
  SkColor color = SkColorSetARGB(255, 11, 22, 33);
  flags.setColor(color);
  SkRect rect = SkRect::MakeWH(100, 100);
  canvas()->translate(1, 1);
  canvas()->drawRect(rect, flags);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_FALSE(IsSolidColor());
}

TEST_F(SolidColorAnalyzerTest, DrawRectWithTranslateSolid) {
  Initialize();
  PaintFlags flags;
  SkColor color = SkColorSetARGB(255, 11, 22, 33);
  flags.setColor(color);
  SkRect rect = SkRect::MakeWH(101, 101);
  canvas()->translate(1, 1);
  canvas()->drawRect(rect, flags);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_FALSE(IsSolidColor());
}

TEST_F(SolidColorAnalyzerTest, TwoOpsNotSolid) {
  Initialize();
  SkColor color = SkColorSetARGB(255, 65, 43, 21);
  canvas()->clear(color);
  canvas()->clear(color);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_FALSE(IsSolidColor());
}

TEST_F(SolidColorAnalyzerTest, DrawRectBlendModeClear) {
  Initialize();
  PaintFlags flags;
  SkColor color = SkColorSetARGB(255, 11, 22, 33);
  flags.setColor(color);
  flags.setBlendMode(SkBlendMode::kClear);
  SkRect rect = SkRect::MakeWH(200, 200);
  canvas()->drawRect(rect, flags);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_EQ(SK_ColorTRANSPARENT, GetColor());
}

TEST_F(SolidColorAnalyzerTest, DrawRectBlendModeSrcOver) {
  Initialize();
  PaintFlags flags;
  SkColor color = SkColorSetARGB(255, 11, 22, 33);
  flags.setColor(color);
  flags.setBlendMode(SkBlendMode::kSrcOver);
  SkRect rect = SkRect::MakeWH(200, 200);
  canvas()->drawRect(rect, flags);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_EQ(color, GetColor());
}

TEST_F(SolidColorAnalyzerTest, DrawRectRotated) {
  Initialize();
  PaintFlags flags;
  SkColor color = SkColorSetARGB(255, 11, 22, 33);
  flags.setColor(color);
  SkRect rect = SkRect::MakeWH(200, 200);
  canvas()->rotate(50);
  canvas()->drawRect(rect, flags);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_FALSE(IsSolidColor());
}

TEST_F(SolidColorAnalyzerTest, DrawRectScaledNotSolid) {
  Initialize();
  PaintFlags flags;
  SkColor color = SkColorSetARGB(255, 11, 22, 33);
  flags.setColor(color);
  SkRect rect = SkRect::MakeWH(200, 200);
  canvas()->scale(0.1, 0.1);
  canvas()->drawRect(rect, flags);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_FALSE(IsSolidColor());
}

TEST_F(SolidColorAnalyzerTest, DrawRectScaledSolid) {
  Initialize();
  PaintFlags flags;
  SkColor color = SkColorSetARGB(255, 11, 22, 33);
  flags.setColor(color);
  SkRect rect = SkRect::MakeWH(10, 10);
  canvas()->scale(10, 10);
  canvas()->drawRect(rect, flags);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_EQ(color, GetColor());
}

TEST_F(SolidColorAnalyzerTest, DrawRectFilterPaint) {
  Initialize();
  PaintFlags flags;
  SkColor color = SkColorSetARGB(255, 11, 22, 33);
  flags.setColor(color);
  flags.setImageFilter(SkOffsetImageFilter::Make(10, 10, nullptr));
  SkRect rect = SkRect::MakeWH(200, 200);
  canvas()->drawRect(rect, flags);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_FALSE(IsSolidColor());
}

TEST_F(SolidColorAnalyzerTest, DrawRectClipPath) {
  Initialize();
  PaintFlags flags;
  SkColor color = SkColorSetARGB(255, 11, 22, 33);
  flags.setColor(color);

  SkPath path;
  path.moveTo(0, 0);
  path.lineTo(128, 50);
  path.lineTo(255, 0);
  path.lineTo(255, 255);
  path.lineTo(0, 255);

  SkRect rect = SkRect::MakeWH(200, 200);
  canvas()->clipPath(path, SkClipOp::kIntersect);
  canvas()->drawRect(rect, flags);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_FALSE(IsSolidColor());
}

TEST_F(SolidColorAnalyzerTest, SaveLayer) {
  Initialize();
  PaintFlags flags;
  SkColor color = SkColorSetARGB(255, 11, 22, 33);
  flags.setColor(color);

  SkRect rect = SkRect::MakeWH(200, 200);
  canvas()->saveLayer(&rect, &flags);
  RunAnalysis(gfx::Rect(0, 0, 100, 100));
  EXPECT_FALSE(IsSolidColor());
}

}  // namespace
}  // namespace cc

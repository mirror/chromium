// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/highlighter/highlighter_gesture_util.h"
#include "ash/test/ash_test_base.h"
#include "ui/gfx/geometry/rect_conversions.h"

namespace ash {

namespace {

const float kLength = 400;

const gfx::RectF kFrame(50, 50, 300, 200);

const gfx::SizeF mPenTipSize(4.0, 14);

const int KPointsPerLine = 10;

float interpolate(float from, float to, int position, int range) {
  return from + (to - from) * position / range;
}

class HighlighterGestureUtilTest : public test::AshTestBase {
 public:
  HighlighterGestureUtilTest() {}

  ~HighlighterGestureUtilTest() override {}

 protected:
  std::vector<gfx::PointF> points_;

  void moveTo(float x, float y) { addPoint(x, y); }

  void lineTo(float x, float y) {
    const gfx::PointF origin = points_.back();
    for (int i = 1; i <= KPointsPerLine; i++) {
      addPoint(interpolate(origin.x(), x, i, KPointsPerLine),
               interpolate(origin.y(), y, i, KPointsPerLine));
    }
  }

  void traceLine(float xFrom, float yFrom, float xTo, float yTo) {
    moveTo(xFrom, yFrom);
    lineTo(xTo, yTo);
  }

  void traceRect(const gfx::RectF& rect, float tilt) {
    moveTo(rect.x() + tilt, rect.y());
    lineTo(rect.right(), rect.y() + tilt);
    lineTo(rect.right() - tilt, rect.bottom());
    lineTo(rect.x(), rect.bottom() - tilt);
    lineTo(rect.x() + tilt, rect.y());
  }

  void traceZigZag(float w, float h) {
    moveTo(0, 0);
    lineTo(w / 4, h);
    lineTo(w / 2, 0);
    lineTo(w * 3 / 4, h);
    lineTo(w, 0);
  }

  bool detectHorizontalStroke() {
    gfx::RectF box = GetBoundingBox(points_);
    return DetectHorizontalStroke(box, mPenTipSize, points_);
  }

  bool detectClosedShape() {
    gfx::RectF box = GetBoundingBox(points_);
    return DetectClosedShape(box, points_);
  }

 private:
  void addPoint(float x, float y) { points_.push_back(gfx::PointF(x, y)); }

  DISALLOW_COPY_AND_ASSIGN(HighlighterGestureUtilTest);
};

}  // namespace

TEST_F(HighlighterGestureUtilTest, HorizontalStrokeTooShort) {
  traceLine(0, 0, 1, 0);
  EXPECT_FALSE(detectHorizontalStroke());
  EXPECT_FALSE(detectClosedShape());
}

TEST_F(HighlighterGestureUtilTest, HorizontalStrokeFlat) {
  traceLine(0, 0, kLength, 0);
  EXPECT_TRUE(detectHorizontalStroke());
  EXPECT_FALSE(detectClosedShape());
}

TEST_F(HighlighterGestureUtilTest, HorizontalStrokeTiltedLeftSlightly) {
  const float tilt = kLength / 20;
  traceLine(0, tilt, kLength, 0);
  EXPECT_TRUE(detectHorizontalStroke());
  EXPECT_FALSE(detectClosedShape());
}

TEST_F(HighlighterGestureUtilTest, HorizontalStrokeTiltedRightSlightly) {
  const float tilt = kLength / 20;
  traceLine(0, 0, kLength, tilt);
  EXPECT_TRUE(detectHorizontalStroke());
  EXPECT_FALSE(detectClosedShape());
}

TEST_F(HighlighterGestureUtilTest, HorizontalStrokeTiltedLeftTooMuch) {
  const float tilt = kLength / 5;
  traceLine(0, tilt, kLength, 0);
  EXPECT_FALSE(detectHorizontalStroke());
  EXPECT_FALSE(detectClosedShape());
}

TEST_F(HighlighterGestureUtilTest, HorizontalStrokeTiltedRightTooMuch) {
  const float tilt = kLength / 5;
  traceLine(0, 0, kLength, tilt);
  EXPECT_FALSE(detectHorizontalStroke());
  EXPECT_FALSE(detectClosedShape());
}

TEST_F(HighlighterGestureUtilTest, HorizontalStrokeZigZagSlightly) {
  const float height = kLength / 20;
  traceZigZag(kLength, height);
  EXPECT_TRUE(detectHorizontalStroke());
  EXPECT_FALSE(detectClosedShape());
}

TEST_F(HighlighterGestureUtilTest, HorizontalStrokeZigZagTooMuch) {
  const float height = kLength / 5;
  traceZigZag(kLength, height);
  EXPECT_FALSE(detectHorizontalStroke());
  EXPECT_FALSE(detectClosedShape());
}

TEST_F(HighlighterGestureUtilTest, ClosedShapeRectangle) {
  traceRect(kFrame, 0);
  EXPECT_FALSE(detectHorizontalStroke());
  EXPECT_TRUE(detectClosedShape());
}

TEST_F(HighlighterGestureUtilTest, ClosedShapeRectangleThin) {
  traceRect(gfx::RectF(0, 0, kLength, kLength / 20), 0);
  EXPECT_TRUE(detectHorizontalStroke());
  EXPECT_TRUE(detectClosedShape());
}

TEST_F(HighlighterGestureUtilTest, ClosedShapeRectangleTilted) {
  traceRect(kFrame, 10.0);
  EXPECT_FALSE(detectHorizontalStroke());
  EXPECT_TRUE(detectClosedShape());
}

TEST_F(HighlighterGestureUtilTest, ClosedShapeWithOverlap) {
  // 1/4 overlap
  moveTo(kFrame.right(), kFrame.y());
  lineTo(kFrame.x(), kFrame.y());
  lineTo(kFrame.x(), kFrame.bottom());
  lineTo(kFrame.right(), kFrame.bottom());
  lineTo(kFrame.right(), kFrame.y());
  lineTo(kFrame.x(), kFrame.y());
  EXPECT_FALSE(detectHorizontalStroke());
  EXPECT_TRUE(detectClosedShape());
}

TEST_F(HighlighterGestureUtilTest, ClosedShapeLikeU) {
  // 1/4 open shape
  moveTo(kFrame.x(), kFrame.y());
  lineTo(kFrame.x(), kFrame.bottom());
  lineTo(kFrame.right(), kFrame.bottom());
  lineTo(kFrame.right(), kFrame.y());
  EXPECT_FALSE(detectHorizontalStroke());
  EXPECT_FALSE(detectClosedShape());
}

TEST_F(HighlighterGestureUtilTest, ClosedShapeLikeG) {
  // 1/8 open shape
  moveTo(kFrame.right(), kFrame.y());
  lineTo(kFrame.x(), kFrame.y());
  lineTo(kFrame.x(), kFrame.bottom());
  lineTo(kFrame.right(), kFrame.bottom());
  lineTo(kFrame.right(), kFrame.CenterPoint().y());
  EXPECT_FALSE(detectHorizontalStroke());
  EXPECT_TRUE(detectClosedShape());
}

TEST_F(HighlighterGestureUtilTest, ClosedShapeWithLittleBackAndForth) {
  moveTo(kFrame.right(), kFrame.y());
  lineTo(kFrame.x(), kFrame.y());
  // Go back one pixel, then go forward again.
  moveTo(kFrame.x() + 1, kFrame.y());
  moveTo(kFrame.x(), kFrame.y());
  lineTo(kFrame.x(), kFrame.bottom());
  lineTo(kFrame.right(), kFrame.bottom());
  lineTo(kFrame.right(), kFrame.CenterPoint().y());
  EXPECT_FALSE(detectHorizontalStroke());
  EXPECT_TRUE(detectClosedShape());
}

TEST_F(HighlighterGestureUtilTest, ClosedShapeWithTooMuchBackAndForth) {
  moveTo(kFrame.right(), kFrame.y());
  lineTo(kFrame.x(), kFrame.y());
  lineTo(kFrame.right(), kFrame.y());
  lineTo(kFrame.x(), kFrame.y());
  lineTo(kFrame.x(), kFrame.bottom());
  lineTo(kFrame.x(), kFrame.y());
  lineTo(kFrame.x(), kFrame.bottom());
  lineTo(kFrame.right(), kFrame.bottom());
  lineTo(kFrame.x(), kFrame.bottom());
  lineTo(kFrame.right(), kFrame.bottom());
  lineTo(kFrame.right(), kFrame.CenterPoint().y());
  EXPECT_FALSE(detectHorizontalStroke());
  EXPECT_FALSE(detectClosedShape());
}

}  // namespace ash

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_op_buffer.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "cc/paint/decoded_draw_image.h"
#include "cc/paint/display_item_list.h"
#include "cc/paint/image_provider.h"
#include "cc/test/paint_op_util.h"
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

using testing::Mock;
using testing::Property;
using testing::_;

TEST(PaintOpBufferTest, Empty) {
  PaintOpBuffer buffer;
  EXPECT_EQ(buffer.size(), 0u);
  EXPECT_EQ(buffer.bytes_used(), sizeof(PaintOpBuffer));
  EXPECT_EQ(PaintOpBuffer::Iterator(&buffer), false);

  buffer.Reset();
  EXPECT_EQ(buffer.size(), 0u);
  EXPECT_EQ(buffer.bytes_used(), sizeof(PaintOpBuffer));
  EXPECT_EQ(PaintOpBuffer::Iterator(&buffer), false);

  PaintOpBuffer buffer2(std::move(buffer));
  EXPECT_EQ(buffer.size(), 0u);
  EXPECT_EQ(buffer.bytes_used(), sizeof(PaintOpBuffer));
  EXPECT_EQ(PaintOpBuffer::Iterator(&buffer), false);
  EXPECT_EQ(buffer2.size(), 0u);
  EXPECT_EQ(buffer2.bytes_used(), sizeof(PaintOpBuffer));
  EXPECT_EQ(PaintOpBuffer::Iterator(&buffer2), false);
}

class PaintOpAppendTest : public ::testing::Test {
 public:
  PaintOpAppendTest() {
    rect_ = SkRect::MakeXYWH(2, 3, 4, 5);
    flags_.setColor(SK_ColorMAGENTA);
    flags_.setAlpha(100);
  }

  void PushOps(PaintOpBuffer* buffer) {
    buffer->push<SaveLayerOp>(&rect_, &flags_);
    buffer->push<SaveOp>();
    buffer->push<DrawColorOp>(draw_color_, blend_);
    buffer->push<RestoreOp>();
    EXPECT_EQ(buffer->size(), 4u);
  }

  void VerifyOps(PaintOpBuffer* buffer) {
    EXPECT_EQ(buffer->size(), 4u);

    PaintOpBuffer::Iterator iter(buffer);
    ASSERT_EQ(iter->GetType(), PaintOpType::SaveLayer);
    SaveLayerOp* save_op = static_cast<SaveLayerOp*>(*iter);
    EXPECT_EQ(save_op->bounds, rect_);
    PaintOpUtil::ExpectEqual(save_op->flags, flags_);
    ++iter;

    ASSERT_EQ(iter->GetType(), PaintOpType::Save);
    ++iter;

    ASSERT_EQ(iter->GetType(), PaintOpType::DrawColor);
    DrawColorOp* op = static_cast<DrawColorOp*>(*iter);
    EXPECT_EQ(op->color, draw_color_);
    EXPECT_EQ(op->mode, blend_);
    ++iter;

    ASSERT_EQ(iter->GetType(), PaintOpType::Restore);
    ++iter;

    EXPECT_FALSE(iter);
  }

 private:
  SkRect rect_;
  PaintFlags flags_;
  SkColor draw_color_ = SK_ColorRED;
  SkBlendMode blend_ = SkBlendMode::kSrc;
};

TEST_F(PaintOpAppendTest, SimpleAppend) {
  PaintOpBuffer buffer;
  PushOps(&buffer);
  VerifyOps(&buffer);

  buffer.Reset();
  PushOps(&buffer);
  VerifyOps(&buffer);
}

TEST_F(PaintOpAppendTest, MoveThenDestruct) {
  PaintOpBuffer original;
  PushOps(&original);
  VerifyOps(&original);

  PaintOpBuffer destination(std::move(original));
  VerifyOps(&destination);

  // Original should be empty, and safe to destruct.
  EXPECT_EQ(original.size(), 0u);
  EXPECT_EQ(original.bytes_used(), sizeof(PaintOpBuffer));
  EXPECT_EQ(PaintOpBuffer::Iterator(&original), false);
}

TEST_F(PaintOpAppendTest, MoveThenDestructOperatorEq) {
  PaintOpBuffer original;
  PushOps(&original);
  VerifyOps(&original);

  PaintOpBuffer destination;
  destination = std::move(original);
  VerifyOps(&destination);

  // Original should be empty, and safe to destruct.
  EXPECT_EQ(original.size(), 0u);
  EXPECT_EQ(original.bytes_used(), sizeof(PaintOpBuffer));
  EXPECT_EQ(PaintOpBuffer::Iterator(&original), false);
}

TEST_F(PaintOpAppendTest, MoveThenReappend) {
  PaintOpBuffer original;
  PushOps(&original);

  PaintOpBuffer destination(std::move(original));

  // Should be possible to reappend to the original and get the same result.
  PushOps(&original);
  VerifyOps(&original);
}

TEST_F(PaintOpAppendTest, MoveThenReappendOperatorEq) {
  PaintOpBuffer original;
  PushOps(&original);

  PaintOpBuffer destination;
  destination = std::move(original);

  // Should be possible to reappend to the original and get the same result.
  PushOps(&original);
  VerifyOps(&original);
}

// Verify that PaintOps with data are stored properly.
TEST(PaintOpBufferTest, PaintOpData) {
  PaintOpBuffer buffer;

  buffer.push<SaveOp>();
  PaintFlags flags;
  char text1[] = "asdfasdf";
  buffer.push_with_data<DrawTextOp>(text1, arraysize(text1), 0.f, 0.f, flags);

  char text2[] = "qwerty";
  buffer.push_with_data<DrawTextOp>(text2, arraysize(text2), 0.f, 0.f, flags);

  ASSERT_EQ(buffer.size(), 3u);

  // Verify iteration behavior and brief smoke test of op state.
  PaintOpBuffer::Iterator iter(&buffer);
  PaintOp* save_op = *iter;
  EXPECT_EQ(save_op->GetType(), PaintOpType::Save);
  ++iter;

  PaintOp* op1 = *iter;
  ASSERT_EQ(op1->GetType(), PaintOpType::DrawText);
  DrawTextOp* draw_text_op1 = static_cast<DrawTextOp*>(op1);
  EXPECT_EQ(draw_text_op1->bytes, arraysize(text1));
  const void* data1 = draw_text_op1->GetData();
  EXPECT_EQ(memcmp(data1, text1, arraysize(text1)), 0);
  ++iter;

  PaintOp* op2 = *iter;
  ASSERT_EQ(op2->GetType(), PaintOpType::DrawText);
  DrawTextOp* draw_text_op2 = static_cast<DrawTextOp*>(op2);
  EXPECT_EQ(draw_text_op2->bytes, arraysize(text2));
  const void* data2 = draw_text_op2->GetData();
  EXPECT_EQ(memcmp(data2, text2, arraysize(text2)), 0);
  ++iter;

  EXPECT_FALSE(iter);
}

// Verify that PaintOps with arrays are stored properly.
TEST(PaintOpBufferTest, PaintOpArray) {
  PaintOpBuffer buffer;
  buffer.push<SaveOp>();

  // arbitrary data
  std::string texts[] = {"xyz", "abcdefg", "thingerdoo"};
  SkPoint point1[] = {SkPoint::Make(1, 2), SkPoint::Make(2, 3),
                      SkPoint::Make(3, 4)};
  SkPoint point2[] = {SkPoint::Make(8, -12)};
  SkPoint point3[] = {SkPoint::Make(0, 0),   SkPoint::Make(5, 6),
                      SkPoint::Make(-1, -1), SkPoint::Make(9, 9),
                      SkPoint::Make(50, 50), SkPoint::Make(100, 100)};
  SkPoint* points[] = {point1, point2, point3};
  size_t counts[] = {arraysize(point1), arraysize(point2), arraysize(point3)};

  for (size_t i = 0; i < arraysize(texts); ++i) {
    PaintFlags flags;
    flags.setAlpha(i);
    buffer.push_with_array<DrawPosTextOp>(texts[i].c_str(), texts[i].length(),
                                          points[i], counts[i], flags);
  }

  PaintOpBuffer::Iterator iter(&buffer);
  PaintOp* save_op = *iter;
  EXPECT_EQ(save_op->GetType(), PaintOpType::Save);
  ++iter;

  for (size_t i = 0; i < arraysize(texts); ++i) {
    ASSERT_EQ(iter->GetType(), PaintOpType::DrawPosText);
    DrawPosTextOp* op = static_cast<DrawPosTextOp*>(*iter);

    EXPECT_EQ(op->flags.getAlpha(), i);

    EXPECT_EQ(op->bytes, texts[i].length());
    const void* data = op->GetData();
    EXPECT_EQ(memcmp(data, texts[i].c_str(), op->bytes), 0);

    EXPECT_EQ(op->count, counts[i]);
    const SkPoint* op_points = op->GetArray();
    for (size_t k = 0; k < op->count; ++k)
      EXPECT_EQ(op_points[k], points[i][k]);

    ++iter;
  }

  EXPECT_FALSE(iter);
}

// Verify that a SaveLayerAlpha / Draw / Restore can be optimized to just
// a draw with opacity.
TEST(PaintOpBufferTest, SaveDrawRestore) {
  PaintOpBuffer buffer;

  uint8_t alpha = 100;
  buffer.push<SaveLayerAlphaOp>(nullptr, alpha, false);

  PaintFlags draw_flags;
  draw_flags.setColor(SK_ColorMAGENTA);
  draw_flags.setAlpha(50);
  EXPECT_TRUE(draw_flags.SupportsFoldingAlpha());
  SkRect rect = SkRect::MakeXYWH(1, 2, 3, 4);
  buffer.push<DrawRectOp>(rect, draw_flags);
  buffer.push<RestoreOp>();

  SaveCountingCanvas canvas;
  buffer.Playback(&canvas);

  EXPECT_EQ(0, canvas.save_count_);
  EXPECT_EQ(0, canvas.restore_count_);
  EXPECT_EQ(rect, canvas.draw_rect_);

  // Expect the alpha from the draw and the save layer to be folded together.
  // Since alpha is stored in a uint8_t and gets rounded, so use tolerance.
  float expected_alpha = alpha * 50 / 255.f;
  EXPECT_LE(std::abs(expected_alpha - canvas.paint_.getAlpha()), 1.f);
}

// The same as SaveDrawRestore, but test that the optimization doesn't apply
// when the drawing op's flags are not compatible with being folded into the
// save layer with opacity.
TEST(PaintOpBufferTest, SaveDrawRestoreFail_BadFlags) {
  PaintOpBuffer buffer;

  uint8_t alpha = 100;
  buffer.push<SaveLayerAlphaOp>(nullptr, alpha, false);

  PaintFlags draw_flags;
  draw_flags.setColor(SK_ColorMAGENTA);
  draw_flags.setAlpha(50);
  draw_flags.setBlendMode(SkBlendMode::kSrc);
  EXPECT_FALSE(draw_flags.SupportsFoldingAlpha());
  SkRect rect = SkRect::MakeXYWH(1, 2, 3, 4);
  buffer.push<DrawRectOp>(rect, draw_flags);
  buffer.push<RestoreOp>();

  SaveCountingCanvas canvas;
  buffer.Playback(&canvas);

  EXPECT_EQ(1, canvas.save_count_);
  EXPECT_EQ(1, canvas.restore_count_);
  EXPECT_EQ(rect, canvas.draw_rect_);
  EXPECT_EQ(draw_flags.getAlpha(), canvas.paint_.getAlpha());
}

// Same as above, but the save layer itself appears to be a noop.
// See: http://crbug.com/748485.  If the inner draw op itself
// doesn't support folding, then the external save can't be skipped.
TEST(PaintOpBufferTest, SaveDrawRestore_BadFlags255Alpha) {
  PaintOpBuffer buffer;

  uint8_t alpha = 255;
  buffer.push<SaveLayerAlphaOp>(nullptr, alpha, false);

  PaintFlags draw_flags;
  draw_flags.setColor(SK_ColorMAGENTA);
  draw_flags.setAlpha(50);
  draw_flags.setBlendMode(SkBlendMode::kColorBurn);
  EXPECT_FALSE(draw_flags.SupportsFoldingAlpha());
  SkRect rect = SkRect::MakeXYWH(1, 2, 3, 4);
  buffer.push<DrawRectOp>(rect, draw_flags);
  buffer.push<RestoreOp>();

  SaveCountingCanvas canvas;
  buffer.Playback(&canvas);

  EXPECT_EQ(1, canvas.save_count_);
  EXPECT_EQ(1, canvas.restore_count_);
  EXPECT_EQ(rect, canvas.draw_rect_);
}

// The same as SaveDrawRestore, but test that the optimization doesn't apply
// when there are more than one ops between the save and restore.
TEST(PaintOpBufferTest, SaveDrawRestoreFail_TooManyOps) {
  PaintOpBuffer buffer;

  uint8_t alpha = 100;
  buffer.push<SaveLayerAlphaOp>(nullptr, alpha, false);

  PaintFlags draw_flags;
  draw_flags.setColor(SK_ColorMAGENTA);
  draw_flags.setAlpha(50);
  draw_flags.setBlendMode(SkBlendMode::kSrcOver);
  EXPECT_TRUE(draw_flags.SupportsFoldingAlpha());
  SkRect rect = SkRect::MakeXYWH(1, 2, 3, 4);
  buffer.push<DrawRectOp>(rect, draw_flags);
  buffer.push<NoopOp>();
  buffer.push<RestoreOp>();

  SaveCountingCanvas canvas;
  buffer.Playback(&canvas);

  EXPECT_EQ(1, canvas.save_count_);
  EXPECT_EQ(1, canvas.restore_count_);
  EXPECT_EQ(rect, canvas.draw_rect_);
  EXPECT_EQ(draw_flags.getAlpha(), canvas.paint_.getAlpha());
}

// Verify that the save draw restore code works with a single op
// that's not a draw op, and the optimization does not kick in.
TEST(PaintOpBufferTest, SaveDrawRestore_SingleOpNotADrawOp) {
  PaintOpBuffer buffer;

  uint8_t alpha = 100;
  buffer.push<SaveLayerAlphaOp>(nullptr, alpha, false);

  buffer.push<NoopOp>();
  buffer.push<RestoreOp>();

  SaveCountingCanvas canvas;
  buffer.Playback(&canvas);

  EXPECT_EQ(1, canvas.save_count_);
  EXPECT_EQ(1, canvas.restore_count_);
}

// Test that the save/draw/restore optimization applies if the single op
// is a DrawRecord that itself has a single draw op.
TEST(PaintOpBufferTest, SaveDrawRestore_SingleOpRecordWithSingleOp) {
  sk_sp<PaintRecord> record = sk_make_sp<PaintRecord>();

  PaintFlags draw_flags;
  draw_flags.setColor(SK_ColorMAGENTA);
  draw_flags.setAlpha(50);
  EXPECT_TRUE(draw_flags.SupportsFoldingAlpha());
  SkRect rect = SkRect::MakeXYWH(1, 2, 3, 4);
  record->push<DrawRectOp>(rect, draw_flags);
  EXPECT_EQ(record->size(), 1u);

  PaintOpBuffer buffer;

  uint8_t alpha = 100;
  buffer.push<SaveLayerAlphaOp>(nullptr, alpha, false);
  buffer.push<DrawRecordOp>(std::move(record));
  buffer.push<RestoreOp>();

  SaveCountingCanvas canvas;
  buffer.Playback(&canvas);

  EXPECT_EQ(0, canvas.save_count_);
  EXPECT_EQ(0, canvas.restore_count_);
  EXPECT_EQ(rect, canvas.draw_rect_);

  float expected_alpha = alpha * 50 / 255.f;
  EXPECT_LE(std::abs(expected_alpha - canvas.paint_.getAlpha()), 1.f);
}

// The same as the above SingleOpRecord test, but the single op is not
// a draw op.  So, there's no way to fold in the save layer optimization.
// Verify that the optimization doesn't apply and that this doesn't crash.
// See: http://crbug.com/712093.
TEST(PaintOpBufferTest, SaveDrawRestore_SingleOpRecordWithSingleNonDrawOp) {
  sk_sp<PaintRecord> record = sk_make_sp<PaintRecord>();
  record->push<NoopOp>();
  EXPECT_EQ(record->size(), 1u);
  EXPECT_FALSE(record->GetFirstOp()->IsDrawOp());

  PaintOpBuffer buffer;

  uint8_t alpha = 100;
  buffer.push<SaveLayerAlphaOp>(nullptr, alpha, false);
  buffer.push<DrawRecordOp>(std::move(record));
  buffer.push<RestoreOp>();

  SaveCountingCanvas canvas;
  buffer.Playback(&canvas);

  EXPECT_EQ(1, canvas.save_count_);
  EXPECT_EQ(1, canvas.restore_count_);
}

TEST(PaintOpBufferTest, SaveLayerRestore_DrawColor) {
  PaintOpBuffer buffer;
  uint8_t alpha = 100;
  SkColor original = SkColorSetA(50, SK_ColorRED);

  buffer.push<SaveLayerAlphaOp>(nullptr, alpha, false);
  buffer.push<DrawColorOp>(original, SkBlendMode::kSrcOver);
  buffer.push<RestoreOp>();

  SaveCountingCanvas canvas;
  buffer.Playback(&canvas);
  EXPECT_EQ(canvas.save_count_, 0);
  EXPECT_EQ(canvas.restore_count_, 0);

  uint8_t expected_alpha = SkMulDiv255Round(alpha, SkColorGetA(original));
  EXPECT_EQ(canvas.paint_.getColor(), SkColorSetA(original, expected_alpha));
}

TEST(PaintOpBufferTest, DiscardableImagesTracking_EmptyBuffer) {
  PaintOpBuffer buffer;
  EXPECT_FALSE(buffer.HasDiscardableImages());
}

TEST(PaintOpBufferTest, DiscardableImagesTracking_NoImageOp) {
  PaintOpBuffer buffer;
  PaintFlags flags;
  buffer.push<DrawRectOp>(SkRect::MakeWH(100, 100), flags);
  EXPECT_FALSE(buffer.HasDiscardableImages());
}

TEST(PaintOpBufferTest, DiscardableImagesTracking_DrawImage) {
  PaintOpBuffer buffer;
  PaintImage image = PaintImage(PaintImage::GetNextId(),
                                CreateDiscardableImage(gfx::Size(100, 100)));
  buffer.push<DrawImageOp>(image, SkIntToScalar(0), SkIntToScalar(0), nullptr);
  EXPECT_TRUE(buffer.HasDiscardableImages());
}

TEST(PaintOpBufferTest, DiscardableImagesTracking_DrawImageRect) {
  PaintOpBuffer buffer;
  PaintImage image = PaintImage(PaintImage::GetNextId(),
                                CreateDiscardableImage(gfx::Size(100, 100)));
  buffer.push<DrawImageRectOp>(
      image, SkRect::MakeWH(100, 100), SkRect::MakeWH(100, 100), nullptr,
      PaintCanvas::SrcRectConstraint::kFast_SrcRectConstraint);
  EXPECT_TRUE(buffer.HasDiscardableImages());
}

TEST(PaintOpBufferTest, DiscardableImagesTracking_OpWithFlags) {
  PaintOpBuffer buffer;
  PaintFlags flags;
  auto image = PaintImage(PaintImage::GetNextId(),
                          CreateDiscardableImage(gfx::Size(100, 100)));
  flags.setShader(PaintShader::MakeImage(std::move(image),
                                         SkShader::kClamp_TileMode,
                                         SkShader::kClamp_TileMode, nullptr));
  buffer.push<DrawRectOp>(SkRect::MakeWH(100, 100), flags);
  EXPECT_TRUE(buffer.HasDiscardableImages());
}

TEST(PaintOpBufferTest, SlowPaths) {
  auto buffer = sk_make_sp<PaintOpBuffer>();
  EXPECT_EQ(buffer->numSlowPaths(), 0);

  // Op without slow paths
  PaintFlags noop_flags;
  SkRect rect = SkRect::MakeXYWH(2, 3, 4, 5);
  buffer->push<SaveLayerOp>(&rect, &noop_flags);

  // Line op with a slow path
  PaintFlags line_effect_slow;
  line_effect_slow.setStrokeWidth(1.f);
  line_effect_slow.setStyle(PaintFlags::kStroke_Style);
  line_effect_slow.setStrokeCap(PaintFlags::kRound_Cap);
  SkScalar intervals[] = {1.f, 1.f};
  line_effect_slow.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));

  buffer->push<DrawLineOp>(1.f, 2.f, 3.f, 4.f, line_effect_slow);
  EXPECT_EQ(buffer->numSlowPaths(), 1);

  // Line effect special case that Skia handles specially.
  PaintFlags line_effect = line_effect_slow;
  line_effect.setStrokeCap(PaintFlags::kButt_Cap);
  buffer->push<DrawLineOp>(1.f, 2.f, 3.f, 4.f, line_effect);
  EXPECT_EQ(buffer->numSlowPaths(), 1);

  // Antialiased convex path is not slow.
  SkPath path;
  path.addCircle(2, 2, 5);
  EXPECT_TRUE(path.isConvex());
  buffer->push<ClipPathOp>(path, SkClipOp::kIntersect, true);
  EXPECT_EQ(buffer->numSlowPaths(), 1);

  // Concave paths are slow only when antialiased.
  SkPath concave = path;
  concave.addCircle(3, 4, 2);
  EXPECT_FALSE(concave.isConvex());
  buffer->push<ClipPathOp>(concave, SkClipOp::kIntersect, true);
  EXPECT_EQ(buffer->numSlowPaths(), 2);
  buffer->push<ClipPathOp>(concave, SkClipOp::kIntersect, false);
  EXPECT_EQ(buffer->numSlowPaths(), 2);

  // Drawing a record with slow paths into another adds the same
  // number of slow paths as the record.
  auto buffer2 = sk_make_sp<PaintOpBuffer>();
  EXPECT_EQ(0, buffer2->numSlowPaths());
  buffer2->push<DrawRecordOp>(buffer);
  EXPECT_EQ(2, buffer2->numSlowPaths());
  buffer2->push<DrawRecordOp>(buffer);
  EXPECT_EQ(4, buffer2->numSlowPaths());
}

TEST(PaintOpBufferTest, NonAAPaint) {
  // PaintOpWithFlags
  {
    auto buffer = sk_make_sp<PaintOpBuffer>();
    EXPECT_FALSE(buffer->HasNonAAPaint());

    // Add a PaintOpWithFlags (in this case a line) with AA.
    PaintFlags line_effect;
    line_effect.setAntiAlias(true);
    buffer->push<DrawLineOp>(1.f, 2.f, 3.f, 4.f, line_effect);
    EXPECT_FALSE(buffer->HasNonAAPaint());

    // Add a PaintOpWithFlags (in this case a line) without AA.
    PaintFlags line_effect_no_aa;
    line_effect_no_aa.setAntiAlias(false);
    buffer->push<DrawLineOp>(1.f, 2.f, 3.f, 4.f, line_effect_no_aa);
    EXPECT_TRUE(buffer->HasNonAAPaint());
  }

  // ClipPathOp
  {
    auto buffer = sk_make_sp<PaintOpBuffer>();
    EXPECT_FALSE(buffer->HasNonAAPaint());

    SkPath path;
    path.addCircle(2, 2, 5);

    // ClipPathOp with AA
    buffer->push<ClipPathOp>(path, SkClipOp::kIntersect, true /* antialias */);
    EXPECT_FALSE(buffer->HasNonAAPaint());

    // ClipPathOp without AA
    buffer->push<ClipPathOp>(path, SkClipOp::kIntersect, false /* antialias */);
    EXPECT_TRUE(buffer->HasNonAAPaint());
  }

  // ClipRRectOp
  {
    auto buffer = sk_make_sp<PaintOpBuffer>();
    EXPECT_FALSE(buffer->HasNonAAPaint());

    // ClipRRectOp with AA
    buffer->push<ClipRRectOp>(SkRRect::MakeEmpty(), SkClipOp::kIntersect,
                              true /* antialias */);
    EXPECT_FALSE(buffer->HasNonAAPaint());

    // ClipRRectOp without AA
    buffer->push<ClipRRectOp>(SkRRect::MakeEmpty(), SkClipOp::kIntersect,
                              false /* antialias */);
    EXPECT_TRUE(buffer->HasNonAAPaint());
  }

  // Drawing a record with non-aa paths into another propogates the value.
  {
    auto buffer = sk_make_sp<PaintOpBuffer>();
    EXPECT_FALSE(buffer->HasNonAAPaint());

    auto sub_buffer = sk_make_sp<PaintOpBuffer>();
    SkPath path;
    path.addCircle(2, 2, 5);
    sub_buffer->push<ClipPathOp>(path, SkClipOp::kIntersect,
                                 false /* antialias */);
    EXPECT_TRUE(sub_buffer->HasNonAAPaint());

    buffer->push<DrawRecordOp>(sub_buffer);
    EXPECT_TRUE(buffer->HasNonAAPaint());
  }

  // The following PaintOpWithFlags types are overridden to *not* ever have
  // non-AA paint. AA is hard to notice, and these kick us out of MSAA in too
  // many cases.

  // DrawImageOp
  {
    auto buffer = sk_make_sp<PaintOpBuffer>();
    EXPECT_FALSE(buffer->HasNonAAPaint());

    PaintImage image = PaintImage(PaintImage::GetNextId(),
                                  CreateDiscardableImage(gfx::Size(100, 100)));
    PaintFlags non_aa_flags;
    non_aa_flags.setAntiAlias(true);
    buffer->push<DrawImageOp>(image, SkIntToScalar(0), SkIntToScalar(0),
                              &non_aa_flags);

    EXPECT_FALSE(buffer->HasNonAAPaint());
  }

  // DrawIRectOp
  {
    auto buffer = sk_make_sp<PaintOpBuffer>();
    EXPECT_FALSE(buffer->HasNonAAPaint());

    PaintFlags non_aa_flags;
    non_aa_flags.setAntiAlias(true);
    buffer->push<DrawIRectOp>(SkIRect::MakeWH(1, 1), non_aa_flags);

    EXPECT_FALSE(buffer->HasNonAAPaint());
  }

  // SaveLayerOp
  {
    auto buffer = sk_make_sp<PaintOpBuffer>();
    EXPECT_FALSE(buffer->HasNonAAPaint());

    PaintFlags non_aa_flags;
    non_aa_flags.setAntiAlias(true);
    auto bounds = SkRect::MakeWH(1, 1);
    buffer->push<SaveLayerOp>(&bounds, &non_aa_flags);

    EXPECT_FALSE(buffer->HasNonAAPaint());
  }
}

class PaintOpBufferOffsetsTest : public ::testing::Test {
 public:
  void SetUp() override {}
  void TearDown() override {
    offsets_.clear();
    buffer_.Reset();
  }

  template <typename T, typename... Args>
  void push_op(Args&&... args) {
    offsets_.push_back(buffer_.next_op_offset());
    buffer_.push<T>(std::forward<Args>(args)...);
  }

  // Returns a subset of offsets_ by selecting only the specified indices.
  std::vector<size_t> Select(const std::vector<size_t>& indices) {
    std::vector<size_t> result;
    for (size_t i : indices)
      result.push_back(offsets_[i]);
    return result;
  }

  void Playback(SkCanvas* canvas, const std::vector<size_t>& offsets) {
    buffer_.Playback(canvas, nullptr, nullptr, &offsets);
  }

 private:
  std::vector<size_t> offsets_;
  PaintOpBuffer buffer_;
};

TEST_F(PaintOpBufferOffsetsTest, ContiguousIndices) {
  MockCanvas canvas;

  push_op<DrawColorOp>(0u, SkBlendMode::kClear);
  push_op<DrawColorOp>(1u, SkBlendMode::kClear);
  push_op<DrawColorOp>(2u, SkBlendMode::kClear);
  push_op<DrawColorOp>(3u, SkBlendMode::kClear);
  push_op<DrawColorOp>(4u, SkBlendMode::kClear);

  // Plays all items.
  testing::Sequence s;
  EXPECT_CALL(canvas, OnDrawPaintWithColor(0u)).InSequence(s);
  EXPECT_CALL(canvas, OnDrawPaintWithColor(1u)).InSequence(s);
  EXPECT_CALL(canvas, OnDrawPaintWithColor(2u)).InSequence(s);
  EXPECT_CALL(canvas, OnDrawPaintWithColor(3u)).InSequence(s);
  EXPECT_CALL(canvas, OnDrawPaintWithColor(4u)).InSequence(s);
  Playback(&canvas, Select({0, 1, 2, 3, 4}));
}

TEST_F(PaintOpBufferOffsetsTest, NonContiguousIndices) {
  MockCanvas canvas;

  push_op<DrawColorOp>(0u, SkBlendMode::kClear);
  push_op<DrawColorOp>(1u, SkBlendMode::kClear);
  push_op<DrawColorOp>(2u, SkBlendMode::kClear);
  push_op<DrawColorOp>(3u, SkBlendMode::kClear);
  push_op<DrawColorOp>(4u, SkBlendMode::kClear);

  // Plays 0, 1, 3, 4 indices.
  testing::Sequence s;
  EXPECT_CALL(canvas, OnDrawPaintWithColor(0u)).InSequence(s);
  EXPECT_CALL(canvas, OnDrawPaintWithColor(1u)).InSequence(s);
  EXPECT_CALL(canvas, OnDrawPaintWithColor(3u)).InSequence(s);
  EXPECT_CALL(canvas, OnDrawPaintWithColor(4u)).InSequence(s);
  Playback(&canvas, Select({0, 1, 3, 4}));
}

TEST_F(PaintOpBufferOffsetsTest, FirstTwoIndices) {
  MockCanvas canvas;

  push_op<DrawColorOp>(0u, SkBlendMode::kClear);
  push_op<DrawColorOp>(1u, SkBlendMode::kClear);
  push_op<DrawColorOp>(2u, SkBlendMode::kClear);
  push_op<DrawColorOp>(3u, SkBlendMode::kClear);
  push_op<DrawColorOp>(4u, SkBlendMode::kClear);

  // Plays first two indices.
  testing::Sequence s;
  EXPECT_CALL(canvas, OnDrawPaintWithColor(0u)).InSequence(s);
  EXPECT_CALL(canvas, OnDrawPaintWithColor(1u)).InSequence(s);
  Playback(&canvas, Select({0, 1}));
}

TEST_F(PaintOpBufferOffsetsTest, MiddleIndex) {
  MockCanvas canvas;

  push_op<DrawColorOp>(0u, SkBlendMode::kClear);
  push_op<DrawColorOp>(1u, SkBlendMode::kClear);
  push_op<DrawColorOp>(2u, SkBlendMode::kClear);
  push_op<DrawColorOp>(3u, SkBlendMode::kClear);
  push_op<DrawColorOp>(4u, SkBlendMode::kClear);

  // Plays index 2.
  testing::Sequence s;
  EXPECT_CALL(canvas, OnDrawPaintWithColor(2u)).InSequence(s);
  Playback(&canvas, Select({2}));
}

TEST_F(PaintOpBufferOffsetsTest, LastTwoElements) {
  MockCanvas canvas;

  push_op<DrawColorOp>(0u, SkBlendMode::kClear);
  push_op<DrawColorOp>(1u, SkBlendMode::kClear);
  push_op<DrawColorOp>(2u, SkBlendMode::kClear);
  push_op<DrawColorOp>(3u, SkBlendMode::kClear);
  push_op<DrawColorOp>(4u, SkBlendMode::kClear);

  // Plays last two elements.
  testing::Sequence s;
  EXPECT_CALL(canvas, OnDrawPaintWithColor(3u)).InSequence(s);
  EXPECT_CALL(canvas, OnDrawPaintWithColor(4u)).InSequence(s);
  Playback(&canvas, Select({3, 4}));
}

TEST_F(PaintOpBufferOffsetsTest, ContiguousIndicesWithSaveLayerAlphaRestore) {
  MockCanvas canvas;

  push_op<DrawColorOp>(0u, SkBlendMode::kClear);
  push_op<DrawColorOp>(1u, SkBlendMode::kClear);
  uint8_t alpha = 100;
  push_op<SaveLayerAlphaOp>(nullptr, alpha, true);
  push_op<RestoreOp>();
  push_op<DrawColorOp>(2u, SkBlendMode::kClear);
  push_op<DrawColorOp>(3u, SkBlendMode::kClear);
  push_op<DrawColorOp>(4u, SkBlendMode::kClear);

  // Items are {0, 1, save, restore, 2, 3, 4}.

  testing::Sequence s;
  EXPECT_CALL(canvas, OnDrawPaintWithColor(0u)).InSequence(s);
  EXPECT_CALL(canvas, OnDrawPaintWithColor(1u)).InSequence(s);
  // The empty SaveLayerAlpha/Restore is dropped.
  EXPECT_CALL(canvas, OnDrawPaintWithColor(2u)).InSequence(s);
  EXPECT_CALL(canvas, OnDrawPaintWithColor(3u)).InSequence(s);
  EXPECT_CALL(canvas, OnDrawPaintWithColor(4u)).InSequence(s);
  Playback(&canvas, Select({0, 1, 2, 3, 4, 5, 6}));
  Mock::VerifyAndClearExpectations(&canvas);
}

TEST_F(PaintOpBufferOffsetsTest,
       NonContiguousIndicesWithSaveLayerAlphaRestore) {
  MockCanvas canvas;

  push_op<DrawColorOp>(0u, SkBlendMode::kClear);
  push_op<DrawColorOp>(1u, SkBlendMode::kClear);
  uint8_t alpha = 100;
  push_op<SaveLayerAlphaOp>(nullptr, alpha, true);
  push_op<DrawColorOp>(2u, SkBlendMode::kClear);
  push_op<DrawColorOp>(3u, SkBlendMode::kClear);
  push_op<RestoreOp>();
  push_op<DrawColorOp>(4u, SkBlendMode::kClear);

  // Items are {0, 1, save, 2, 3, restore, 4}.

  // Plays back all indices.
  {
    testing::Sequence s;
    EXPECT_CALL(canvas, OnDrawPaintWithColor(0u)).InSequence(s);
    EXPECT_CALL(canvas, OnDrawPaintWithColor(1u)).InSequence(s);
    // The SaveLayerAlpha/Restore is not dropped if we draw the middle
    // range, as we need them to represent the two draws inside the layer
    // correctly.
    EXPECT_CALL(canvas, OnSaveLayer()).InSequence(s);
    EXPECT_CALL(canvas, OnDrawPaintWithColor(2u)).InSequence(s);
    EXPECT_CALL(canvas, OnDrawPaintWithColor(3u)).InSequence(s);
    EXPECT_CALL(canvas, willRestore()).InSequence(s);
    EXPECT_CALL(canvas, OnDrawPaintWithColor(4u)).InSequence(s);
    Playback(&canvas, Select({0, 1, 2, 3, 4, 5, 6}));
  }
  Mock::VerifyAndClearExpectations(&canvas);

  // Skips the middle indices.
  {
    testing::Sequence s;
    EXPECT_CALL(canvas, OnDrawPaintWithColor(0u)).InSequence(s);
    EXPECT_CALL(canvas, OnDrawPaintWithColor(1u)).InSequence(s);
    // The now-empty SaveLayerAlpha/Restore is dropped
    EXPECT_CALL(canvas, OnDrawPaintWithColor(4u)).InSequence(s);
    Playback(&canvas, Select({0, 1, 2, 5, 6}));
  }
  Mock::VerifyAndClearExpectations(&canvas);
}

TEST_F(PaintOpBufferOffsetsTest,
       ContiguousIndicesWithSaveLayerAlphaDrawRestore) {
  MockCanvas canvas;

  auto add_draw_rect = [this](SkColor c) {
    PaintFlags flags;
    flags.setColor(c);
    push_op<DrawRectOp>(SkRect::MakeWH(1, 1), flags);
  };

  add_draw_rect(0u);
  add_draw_rect(1u);
  uint8_t alpha = 100;
  push_op<SaveLayerAlphaOp>(nullptr, alpha, true);
  add_draw_rect(2u);
  push_op<RestoreOp>();
  add_draw_rect(3u);
  add_draw_rect(4u);

  // Items are {0, 1, save, 2, restore, 3, 4}.

  testing::Sequence s;
  EXPECT_CALL(canvas, OnDrawRectWithColor(0u)).InSequence(s);
  EXPECT_CALL(canvas, OnDrawRectWithColor(1u)).InSequence(s);
  // The empty SaveLayerAlpha/Restore is dropped, the containing
  // operation can be drawn with alpha.
  EXPECT_CALL(canvas, OnDrawRectWithColor(2u)).InSequence(s);
  EXPECT_CALL(canvas, OnDrawRectWithColor(3u)).InSequence(s);
  EXPECT_CALL(canvas, OnDrawRectWithColor(4u)).InSequence(s);
  Playback(&canvas, Select({0, 1, 2, 3, 4, 5, 6}));
  Mock::VerifyAndClearExpectations(&canvas);
}

TEST_F(PaintOpBufferOffsetsTest,
       NonContiguousIndicesWithSaveLayerAlphaDrawRestore) {
  MockCanvas canvas;

  auto add_draw_rect = [this](SkColor c) {
    PaintFlags flags;
    flags.setColor(c);
    push_op<DrawRectOp>(SkRect::MakeWH(1, 1), flags);
  };

  add_draw_rect(0u);
  add_draw_rect(1u);
  uint8_t alpha = 100;
  push_op<SaveLayerAlphaOp>(nullptr, alpha, true);
  add_draw_rect(2u);
  add_draw_rect(3u);
  add_draw_rect(4u);
  push_op<RestoreOp>();

  // Items are are {0, 1, save, 2, 3, 4, restore}.

  // If the middle range is played, then the SaveLayerAlpha/Restore
  // can't be dropped.
  {
    testing::Sequence s;
    EXPECT_CALL(canvas, OnDrawRectWithColor(0u)).InSequence(s);
    EXPECT_CALL(canvas, OnDrawRectWithColor(1u)).InSequence(s);
    EXPECT_CALL(canvas, OnSaveLayer()).InSequence(s);
    EXPECT_CALL(canvas, OnDrawRectWithColor(2u)).InSequence(s);
    EXPECT_CALL(canvas, OnDrawRectWithColor(3u)).InSequence(s);
    EXPECT_CALL(canvas, OnDrawRectWithColor(4u)).InSequence(s);
    EXPECT_CALL(canvas, willRestore()).InSequence(s);
    Playback(&canvas, Select({0, 1, 2, 3, 4, 5, 6}));
  }
  Mock::VerifyAndClearExpectations(&canvas);

  // If the middle range is not played, then the SaveLayerAlpha/Restore
  // can be dropped.
  {
    testing::Sequence s;
    EXPECT_CALL(canvas, OnDrawRectWithColor(0u)).InSequence(s);
    EXPECT_CALL(canvas, OnDrawRectWithColor(1u)).InSequence(s);
    EXPECT_CALL(canvas, OnDrawRectWithColor(4u)).InSequence(s);
    Playback(&canvas, Select({0, 1, 2, 5, 6}));
  }
  Mock::VerifyAndClearExpectations(&canvas);

  // If the middle range is not played, then the SaveLayerAlpha/Restore
  // can be dropped.
  {
    testing::Sequence s;
    EXPECT_CALL(canvas, OnDrawRectWithColor(0u)).InSequence(s);
    EXPECT_CALL(canvas, OnDrawRectWithColor(1u)).InSequence(s);
    EXPECT_CALL(canvas, OnDrawRectWithColor(2u)).InSequence(s);
    Playback(&canvas, Select({0, 1, 2, 3, 6}));
  }
}

TEST(PaintOpBufferTest, SaveLayerAlphaDrawRestoreWithBadBlendMode) {
  PaintOpBuffer buffer;
  MockCanvas canvas;

  auto add_draw_rect = [](PaintOpBuffer* buffer, SkColor c) {
    PaintFlags flags;
    flags.setColor(c);
    // This blend mode prevents the optimization.
    flags.setBlendMode(SkBlendMode::kSrc);
    buffer->push<DrawRectOp>(SkRect::MakeWH(1, 1), flags);
  };

  add_draw_rect(&buffer, 0u);
  uint8_t alpha = 100;
  buffer.push<SaveLayerAlphaOp>(nullptr, alpha, true);
  add_draw_rect(&buffer, 1u);
  buffer.push<RestoreOp>();
  add_draw_rect(&buffer, 2u);

  {
    testing::Sequence s;
    EXPECT_CALL(canvas, OnDrawRectWithColor(0u)).InSequence(s);
    EXPECT_CALL(canvas, OnSaveLayer()).InSequence(s);
    EXPECT_CALL(canvas, OnDrawRectWithColor(1u)).InSequence(s);
    EXPECT_CALL(canvas, willRestore()).InSequence(s);
    EXPECT_CALL(canvas, OnDrawRectWithColor(2u)).InSequence(s);
    buffer.Playback(&canvas);
  }
}

TEST(PaintOpBufferTest, UnmatchedSaveRestoreNoSideEffects) {
  PaintOpBuffer buffer;
  MockCanvas canvas;

  auto add_draw_rect = [](PaintOpBuffer* buffer, SkColor c) {
    PaintFlags flags;
    flags.setColor(c);
    buffer->push<DrawRectOp>(SkRect::MakeWH(1, 1), flags);
  };

  // Push 2 saves.

  uint8_t alpha = 100;
  buffer.push<SaveLayerAlphaOp>(nullptr, alpha, true);
  add_draw_rect(&buffer, 0u);
  buffer.push<SaveLayerAlphaOp>(nullptr, alpha, true);
  add_draw_rect(&buffer, 1u);
  add_draw_rect(&buffer, 2u);
  // But only 1 restore.
  buffer.push<RestoreOp>();

  testing::Sequence s;
  EXPECT_CALL(canvas, OnSaveLayer()).InSequence(s);
  EXPECT_CALL(canvas, OnDrawRectWithColor(0u)).InSequence(s);
  EXPECT_CALL(canvas, OnSaveLayer()).InSequence(s);
  EXPECT_CALL(canvas, OnDrawRectWithColor(1u)).InSequence(s);
  EXPECT_CALL(canvas, OnDrawRectWithColor(2u)).InSequence(s);
  EXPECT_CALL(canvas, willRestore()).InSequence(s);
  // We will restore back to the original save count regardless with 2 restores.
  EXPECT_CALL(canvas, willRestore()).InSequence(s);
  buffer.Playback(&canvas);
}

TEST(PaintOpBufferTest, BoundingRect_DrawArcOp) {
  PaintOpBuffer buffer;
  PaintOpUtil::PushDrawArcOps(&buffer);

  SkRect rect;
  for (auto* base_op : PaintOpBuffer::Iterator(&buffer)) {
    auto* op = static_cast<DrawArcOp*>(base_op);

    ASSERT_TRUE(PaintOp::GetBounds(op, &rect));
    EXPECT_EQ(rect, op->oval.makeSorted());
  }
}

TEST(PaintOpBufferTest, BoundingRect_DrawCircleOp) {
  PaintOpBuffer buffer;
  PaintFlags flags;
  buffer.push<DrawCircleOp>(0.f, 0.f, 5.f, flags);
  buffer.push<DrawCircleOp>(-1.f, 4.f, 44.f, flags);
  buffer.push<DrawCircleOp>(-99.f, -32.f, 100.f, flags);

  SkRect rect;
  for (auto* base_op : PaintOpBuffer::Iterator(&buffer)) {
    auto* op = static_cast<DrawCircleOp*>(base_op);

    SkScalar dimension = 2 * op->radius;
    SkScalar x = op->cx - op->radius;
    SkScalar y = op->cy - op->radius;
    SkRect circle_rect = SkRect::MakeXYWH(x, y, dimension, dimension);

    ASSERT_TRUE(PaintOp::GetBounds(op, &rect));
    EXPECT_EQ(rect, circle_rect.makeSorted());
  }
}

TEST(PaintOpBufferTest, BoundingRect_DrawImageOp) {
  PaintOpBuffer buffer;
  PaintOpUtil::PushDrawImageOps(&buffer);

  SkRect rect;
  for (auto* base_op : PaintOpBuffer::Iterator(&buffer)) {
    auto* op = static_cast<DrawImageOp*>(base_op);

    SkRect image_rect =
        SkRect::MakeXYWH(op->left, op->top, op->image.GetSkImage()->width(),
                         op->image.GetSkImage()->height());
    ASSERT_TRUE(PaintOp::GetBounds(op, &rect));
    EXPECT_EQ(rect, image_rect.makeSorted());
  }
}

TEST(PaintOpBufferTest, BoundingRect_DrawImageRectOp) {
  PaintOpBuffer buffer;
  PaintOpUtil::PushDrawImageRectOps(&buffer);

  SkRect rect;
  for (auto* base_op : PaintOpBuffer::Iterator(&buffer)) {
    auto* op = static_cast<DrawImageRectOp*>(base_op);

    ASSERT_TRUE(PaintOp::GetBounds(op, &rect));
    EXPECT_EQ(rect, op->dst.makeSorted());
  }
}

TEST(PaintOpBufferTest, BoundingRect_DrawIRectOp) {
  PaintOpBuffer buffer;
  PaintOpUtil::PushDrawIRectOps(&buffer);

  SkRect rect;
  for (auto* base_op : PaintOpBuffer::Iterator(&buffer)) {
    auto* op = static_cast<DrawIRectOp*>(base_op);

    ASSERT_TRUE(PaintOp::GetBounds(op, &rect));
    EXPECT_EQ(rect, SkRect::Make(op->rect).makeSorted());
  }
}

TEST(PaintOpBufferTest, BoundingRect_DrawOvalOp) {
  PaintOpBuffer buffer;
  PaintOpUtil::PushDrawOvalOps(&buffer);

  SkRect rect;
  for (auto* base_op : PaintOpBuffer::Iterator(&buffer)) {
    auto* op = static_cast<DrawOvalOp*>(base_op);

    ASSERT_TRUE(PaintOp::GetBounds(op, &rect));
    EXPECT_EQ(rect, op->oval.makeSorted());
  }
}

TEST(PaintOpBufferTest, BoundingRect_DrawPathOp) {
  PaintOpBuffer buffer;
  PaintOpUtil::PushDrawPathOps(&buffer);

  SkRect rect;
  for (auto* base_op : PaintOpBuffer::Iterator(&buffer)) {
    auto* op = static_cast<DrawPathOp*>(base_op);

    ASSERT_TRUE(PaintOp::GetBounds(op, &rect));
    EXPECT_EQ(rect, op->path.getBounds().makeSorted());
  }
}

TEST(PaintOpBufferTest, BoundingRect_DrawRectOp) {
  PaintOpBuffer buffer;
  PaintOpUtil::PushDrawRectOps(&buffer);

  SkRect rect;
  for (auto* base_op : PaintOpBuffer::Iterator(&buffer)) {
    auto* op = static_cast<DrawRectOp*>(base_op);

    ASSERT_TRUE(PaintOp::GetBounds(op, &rect));
    EXPECT_EQ(rect, op->rect.makeSorted());
  }
}

TEST(PaintOpBufferTest, BoundingRect_DrawRRectOp) {
  PaintOpBuffer buffer;
  PaintOpUtil::PushDrawRRectOps(&buffer);

  SkRect rect;
  for (auto* base_op : PaintOpBuffer::Iterator(&buffer)) {
    auto* op = static_cast<DrawRRectOp*>(base_op);

    ASSERT_TRUE(PaintOp::GetBounds(op, &rect));
    EXPECT_EQ(rect, op->rrect.rect().makeSorted());
  }
}

TEST(PaintOpBufferTest, BoundingRect_DrawLineOp) {
  PaintOpBuffer buffer;
  PaintOpUtil::PushDrawLineOps(&buffer);

  SkRect rect;
  for (auto* base_op : PaintOpBuffer::Iterator(&buffer)) {
    auto* op = static_cast<DrawLineOp*>(base_op);

    SkRect line_rect;
    line_rect.fLeft = op->x0;
    line_rect.fTop = op->y0;
    line_rect.fRight = op->x1;
    line_rect.fBottom = op->y1;
    ASSERT_TRUE(PaintOp::GetBounds(op, &rect));
    EXPECT_EQ(rect, line_rect.makeSorted());
  }
}

TEST(PaintOpBufferTest, BoundingRect_DrawDRRectOp) {
  PaintOpBuffer buffer;
  PaintOpUtil::PushDrawDRRectOps(&buffer);

  SkRect rect;
  for (auto* base_op : PaintOpBuffer::Iterator(&buffer)) {
    auto* op = static_cast<DrawDRRectOp*>(base_op);

    ASSERT_TRUE(PaintOp::GetBounds(op, &rect));
    EXPECT_EQ(rect, op->outer.getBounds().makeSorted());
  }
}

TEST(PaintOpBufferTest, BoundingRect_DrawTextBlobOp) {
  PaintOpBuffer buffer;
  PaintOpUtil::PushDrawTextBlobOps(&buffer);

  SkRect rect;
  for (auto* base_op : PaintOpBuffer::Iterator(&buffer)) {
    auto* op = static_cast<DrawTextBlobOp*>(base_op);

    ASSERT_TRUE(PaintOp::GetBounds(op, &rect));
    EXPECT_EQ(rect, op->blob->bounds().makeOffset(op->x, op->y).makeSorted());
  }
}

class MockImageProvider : public ImageProvider {
 public:
  MockImageProvider() = default;
  MockImageProvider(std::vector<SkSize> src_rect_offset,
                    std::vector<SkSize> scale,
                    std::vector<SkFilterQuality> quality)
      : src_rect_offset_(src_rect_offset), scale_(scale), quality_(quality) {}

  ~MockImageProvider() override = default;

  ScopedDecodedDrawImage GetDecodedDrawImage(const PaintImage& paint_image,
                                             const SkRect& src_rect,
                                             SkFilterQuality filter_quality,
                                             const SkMatrix& matrix) override {
    SkBitmap bitmap;
    bitmap.allocN32Pixels(10, 10);
    sk_sp<SkImage> image = SkImage::MakeFromBitmap(bitmap);
    size_t i = index_++;
    return ScopedDecodedDrawImage(
        DecodedDrawImage(image, src_rect_offset_[i], scale_[i], quality_[i]));
  }

 private:
  std::vector<SkSize> src_rect_offset_;
  std::vector<SkSize> scale_;
  std::vector<SkFilterQuality> quality_;
  size_t index_ = 0;
};

TEST(PaintOpBufferTest, SkipsOpsOutsideClip) {
  // All ops with images draw outside the clip and should be skipped. If any
  // call is made to the ImageProvider, it should crash.
  MockImageProvider image_provider;
  PaintOpBuffer buffer;

  // Apply a clip outside the region for images.
  buffer.push<ClipRectOp>(SkRect::MakeXYWH(0, 0, 100, 100),
                          SkClipOp::kIntersect, false);

  SkRect rect = SkRect::MakeXYWH(0, 0, 100, 100);
  uint8_t alpha = 220;
  buffer.push<SaveLayerAlphaOp>(&rect, alpha, false);

  PaintFlags flags;
  PaintImage paint_image = PaintImage(
      PaintImage::GetNextId(), CreateDiscardableImage(gfx::Size(10, 10)));
  buffer.push<DrawImageOp>(paint_image, 105.0f, 105.0f, &flags);
  PaintFlags image_flags;
  image_flags.setShader(
      PaintShader::MakeImage(paint_image, SkShader::TileMode::kRepeat_TileMode,
                             SkShader::TileMode::kRepeat_TileMode, nullptr));
  buffer.push<DrawRectOp>(SkRect::MakeXYWH(110, 110, 100, 100), image_flags);

  buffer.push<DrawRectOp>(rect, PaintFlags());
  buffer.push<RestoreOp>();

  // Using a strict mock to ensure that skipping image ops optimizes the
  // save/restore sequences. The single save/restore call is from the
  // PaintOpBuffer's use of SkAutoRestoreCanvas.
  testing::StrictMock<MockCanvas> canvas;
  testing::Sequence s;
  EXPECT_CALL(canvas, willSave()).InSequence(s);
  EXPECT_CALL(canvas, OnDrawRectWithColor(_)).InSequence(s);
  EXPECT_CALL(canvas, willRestore()).InSequence(s);
  buffer.Playback(&canvas, &image_provider);
}

MATCHER(NonLazyImage, "") {
  return !arg->isLazyGenerated();
}

MATCHER_P(MatchesInvScale, expected, "") {
  SkSize scale;
  arg.decomposeScale(&scale, nullptr);
  SkSize inv = SkSize::Make(1.0f / scale.width(), 1.0f / scale.height());
  return inv == expected;
}

MATCHER_P2(MatchesRect, rect, scale, "") {
  EXPECT_EQ(arg->x(), rect.x() * scale.width());
  EXPECT_EQ(arg->y(), rect.y() * scale.height());
  EXPECT_EQ(arg->width(), rect.width() * scale.width());
  EXPECT_EQ(arg->height(), rect.height() * scale.height());
  return true;
}

MATCHER_P(MatchesQuality, quality, "") {
  return quality == arg->getFilterQuality();
}

MATCHER_P2(MatchesShader, flags, scale, "") {
  SkMatrix matrix;
  SkShader::TileMode xy[2];
  SkImage* image = arg.getShader()->isAImage(&matrix, xy);

  EXPECT_FALSE(image->isLazyGenerated());

  SkSize local_scale;
  matrix.decomposeScale(&local_scale, nullptr);
  EXPECT_EQ(local_scale.width(), 1.0f / scale.width());
  EXPECT_EQ(local_scale.height(), 1.0f / scale.height());

  EXPECT_EQ(flags.getShader()->tx(), xy[0]);
  EXPECT_EQ(flags.getShader()->ty(), xy[1]);

  return true;
};

TEST(PaintOpBufferTest, ReplacesImagesFromProvider) {
  std::vector<SkSize> src_rect_offset = {
      SkSize::MakeEmpty(), SkSize::Make(2.0f, 2.0f), SkSize::Make(3.0f, 3.0f)};
  std::vector<SkSize> scale_adjustment = {SkSize::Make(0.2f, 0.2f),
                                          SkSize::Make(0.3f, 0.3f),
                                          SkSize::Make(0.4f, 0.4f)};
  std::vector<SkFilterQuality> quality = {
      kHigh_SkFilterQuality, kMedium_SkFilterQuality, kHigh_SkFilterQuality};

  MockImageProvider image_provider(src_rect_offset, scale_adjustment, quality);
  PaintOpBuffer buffer;

  SkRect rect = SkRect::MakeWH(10, 10);
  PaintFlags flags;
  flags.setFilterQuality(kLow_SkFilterQuality);
  PaintImage paint_image = PaintImage(
      PaintImage::GetNextId(), CreateDiscardableImage(gfx::Size(10, 10)));
  buffer.push<DrawImageOp>(paint_image, 0.0f, 0.0f, &flags);
  buffer.push<DrawImageRectOp>(
      paint_image, rect, rect, &flags,
      PaintCanvas::SrcRectConstraint::kFast_SrcRectConstraint);
  flags.setShader(
      PaintShader::MakeImage(paint_image, SkShader::TileMode::kRepeat_TileMode,
                             SkShader::TileMode::kRepeat_TileMode, nullptr));
  buffer.push<DrawOvalOp>(SkRect::MakeWH(10, 10), flags);

  testing::StrictMock<MockCanvas> canvas;
  testing::Sequence s;

  // Save/scale/image/restore from DrawImageop.
  EXPECT_CALL(canvas, willSave()).InSequence(s);
  EXPECT_CALL(canvas, didConcat(MatchesInvScale(scale_adjustment[0])));
  EXPECT_CALL(canvas, onDrawImage(NonLazyImage(), 0.0f, 0.0f,
                                  MatchesQuality(quality[0])));
  EXPECT_CALL(canvas, willRestore()).InSequence(s);

  // DrawImageRectop.
  SkRect src_rect =
      rect.makeOffset(src_rect_offset[1].width(), src_rect_offset[1].height());
  EXPECT_CALL(canvas,
              onDrawImageRect(
                  NonLazyImage(), MatchesRect(src_rect, scale_adjustment[1]),
                  SkRect::MakeWH(10, 10), MatchesQuality(quality[1]),
                  SkCanvas::kFast_SrcRectConstraint));

  // DrawOvalop.
  EXPECT_CALL(canvas, onDrawOval(SkRect::MakeWH(10, 10),
                                 MatchesShader(flags, scale_adjustment[2])));

  buffer.Playback(&canvas, &image_provider);
}

}  // namespace cc

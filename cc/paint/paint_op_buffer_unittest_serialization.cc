// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "cc/paint/decoded_draw_image.h"
#include "cc/paint/display_item_list.h"
#include "cc/paint/image_provider.h"
#include "cc/paint/paint_op_buffer.h"
#include "cc/paint/paint_op_reader.h"
#include "cc/paint/paint_op_writer.h"
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
namespace {
// An arbitrary size guaranteed to fit the size of any serialized op in this
// unit test.  This can also be used for deserialized op size safely in this
// unit test suite as generally deserialized ops are smaller.
static constexpr size_t kBufferBytesPerOp = 1000 + sizeof(LargestPaintOp);
}  // namespace

// Writes as many ops in |buffer| as can fit in |output_size| to |output|.
// Records the numbers of bytes written for each op.
class SimpleSerializer {
 public:
  SimpleSerializer(void* output, size_t output_size)
      : current_(static_cast<char*>(output)),
        output_size_(output_size),
        remaining_(output_size) {}

  void Serialize(const PaintOpBuffer& buffer) {
    bytes_written_.resize(buffer.size());
    for (size_t i = 0; i < buffer.size(); ++i)
      bytes_written_[i] = 0;

    PaintOp::SerializeOptions options;

    size_t op_idx = 0;
    for (const auto* op : PaintOpBuffer::Iterator(&buffer)) {
      size_t bytes_written = op->Serialize(current_, remaining_, options);
      if (!bytes_written)
        return;

      PaintOp* written = reinterpret_cast<PaintOp*>(current_);
      EXPECT_EQ(op->GetType(), written->GetType());
      EXPECT_EQ(bytes_written, written->skip);

      bytes_written_[op_idx] = bytes_written;
      op_idx++;
      current_ += bytes_written;
      remaining_ -= bytes_written;

      // Number of bytes bytes_written must be a multiple of PaintOpAlign
      // unless the buffer is filled entirely.
      if (remaining_ != 0u)
        DCHECK_EQ(0u, bytes_written % PaintOpBuffer::PaintOpAlign);
    }
  }

  const std::vector<size_t>& bytes_written() const { return bytes_written_; }
  size_t TotalBytesWritten() const { return output_size_ - remaining_; }

 private:
  char* current_ = nullptr;
  size_t output_size_ = 0u;
  size_t remaining_ = 0u;
  std::vector<size_t> bytes_written_;
};

class DeserializerIterator {
 public:
  DeserializerIterator(const void* input, size_t input_size)
      : DeserializerIterator(input,
                             static_cast<const char*>(input),
                             input_size,
                             input_size) {}

  DeserializerIterator(DeserializerIterator&&) = default;
  DeserializerIterator& operator=(DeserializerIterator&&) = default;

  ~DeserializerIterator() { DestroyDeserializedOp(); }

  DeserializerIterator begin() {
    return DeserializerIterator(input_, static_cast<const char*>(input_),
                                input_size_, input_size_);
  }
  DeserializerIterator end() {
    return DeserializerIterator(
        input_, static_cast<const char*>(input_) + input_size_, input_size_, 0);
  }
  bool operator!=(const DeserializerIterator& other) {
    return input_ != other.input_ || current_ != other.current_ ||
           input_size_ != other.input_size_ || remaining_ != other.remaining_;
  }
  DeserializerIterator& operator++() {
    const PaintOp* serialized = reinterpret_cast<const PaintOp*>(current_);

    CHECK_GE(remaining_, serialized->skip);
    current_ += serialized->skip;
    remaining_ -= serialized->skip;

    if (remaining_ > 0)
      CHECK_GE(remaining_, 4u);

    DeserializeCurrentOp();

    return *this;
  }

  operator bool() const { return remaining_ == 0u; }
  const PaintOp* operator->() const { return deserialized_op_; }
  const PaintOp* operator*() const { return deserialized_op_; }

 private:
  DeserializerIterator(const void* input,
                       const char* current,
                       size_t input_size,
                       size_t remaining)
      : input_(input),
        current_(current),
        input_size_(input_size),
        remaining_(remaining) {
    DeserializeCurrentOp();
  }

  void DestroyDeserializedOp() {
    if (!deserialized_op_)
      return;
    deserialized_op_->DestroyThis();
    deserialized_op_ = nullptr;
  }

  void DeserializeCurrentOp() {
    DestroyDeserializedOp();

    if (!remaining_)
      return;

    const PaintOp* serialized = reinterpret_cast<const PaintOp*>(current_);
    size_t required = sizeof(LargestPaintOp) + serialized->skip;

    if (data_size_ < required) {
      data_.reset(static_cast<char*>(
          base::AlignedAlloc(required, PaintOpBuffer::PaintOpAlign)));
      data_size_ = required;
    }
    deserialized_op_ =
        PaintOp::Deserialize(current_, remaining_, data_.get(), data_size_);
  }

  const void* input_ = nullptr;
  const char* current_ = nullptr;
  size_t input_size_ = 0u;
  size_t remaining_ = 0u;
  std::unique_ptr<char, base::AlignedFreeDeleter> data_;
  size_t data_size_ = 0u;
  PaintOp* deserialized_op_ = nullptr;
};

class PaintOpSerializationTest : public ::testing::TestWithParam<uint8_t> {
 public:
  PaintOpSerializationTest() {
    // Verify test data.
    const auto& test_rrects = PaintOpUtil::TestRRects();
    for (size_t i = 0; i < test_rrects.size(); ++i)
      EXPECT_TRUE(test_rrects[i].isValid());
  }

  PaintOpType GetParamType() const {
    return static_cast<PaintOpType>(GetParam());
  }

  void PushTestOps(PaintOpType type) {
    switch (type) {
      case PaintOpType::Annotate:
        PaintOpUtil::PushAnnotateOps(&buffer_);
        break;
      case PaintOpType::ClipDeviceRect:
        PaintOpUtil::PushClipDeviceRectOps(&buffer_);
        break;
      case PaintOpType::ClipPath:
        PaintOpUtil::PushClipPathOps(&buffer_);
        break;
      case PaintOpType::ClipRect:
        PaintOpUtil::PushClipRectOps(&buffer_);
        break;
      case PaintOpType::ClipRRect:
        PaintOpUtil::PushClipRRectOps(&buffer_);
        break;
      case PaintOpType::Concat:
        PaintOpUtil::PushConcatOps(&buffer_);
        break;
      case PaintOpType::DrawArc:
        PaintOpUtil::PushDrawArcOps(&buffer_);
        break;
      case PaintOpType::DrawCircle:
        PaintOpUtil::PushDrawCircleOps(&buffer_);
        break;
      case PaintOpType::DrawColor:
        PaintOpUtil::PushDrawColorOps(&buffer_);
        break;
      case PaintOpType::DrawDRRect:
        PaintOpUtil::PushDrawDRRectOps(&buffer_);
        break;
      case PaintOpType::DrawImage:
        PaintOpUtil::PushDrawImageOps(&buffer_);
        break;
      case PaintOpType::DrawImageRect:
        PaintOpUtil::PushDrawImageRectOps(&buffer_);
        break;
      case PaintOpType::DrawIRect:
        PaintOpUtil::PushDrawIRectOps(&buffer_);
        break;
      case PaintOpType::DrawLine:
        PaintOpUtil::PushDrawLineOps(&buffer_);
        break;
      case PaintOpType::DrawOval:
        PaintOpUtil::PushDrawOvalOps(&buffer_);
        break;
      case PaintOpType::DrawPath:
        PaintOpUtil::PushDrawPathOps(&buffer_);
        break;
      case PaintOpType::DrawPosText:
        PaintOpUtil::PushDrawPosTextOps(&buffer_);
        break;
      case PaintOpType::DrawRecord:
        // Not supported.
        break;
      case PaintOpType::DrawRect:
        PaintOpUtil::PushDrawRectOps(&buffer_);
        break;
      case PaintOpType::DrawRRect:
        PaintOpUtil::PushDrawRRectOps(&buffer_);
        break;
      case PaintOpType::DrawText:
        PaintOpUtil::PushDrawTextOps(&buffer_);
        break;
      case PaintOpType::DrawTextBlob:
        PaintOpUtil::PushDrawTextBlobOps(&buffer_);
        break;
      case PaintOpType::Noop:
        PaintOpUtil::PushNoopOps(&buffer_);
        break;
      case PaintOpType::Restore:
        PaintOpUtil::PushRestoreOps(&buffer_);
        break;
      case PaintOpType::Rotate:
        PaintOpUtil::PushRotateOps(&buffer_);
        break;
      case PaintOpType::Save:
        PaintOpUtil::PushSaveOps(&buffer_);
        break;
      case PaintOpType::SaveLayer:
        PaintOpUtil::PushSaveLayerOps(&buffer_);
        break;
      case PaintOpType::SaveLayerAlpha:
        PaintOpUtil::PushSaveLayerAlphaOps(&buffer_);
        break;
      case PaintOpType::Scale:
        PaintOpUtil::PushScaleOps(&buffer_);
        break;
      case PaintOpType::SetMatrix:
        PaintOpUtil::PushSetMatrixOps(&buffer_);
        break;
      case PaintOpType::Translate:
        PaintOpUtil::PushTranslateOps(&buffer_);
        break;
    }
  }

  void ResizeOutputBuffer() {
    // An arbitrary deserialization buffer size that should fit all the ops
    // in the buffer_.
    output_size_ = kBufferBytesPerOp * buffer_.size();
    output_.reset(static_cast<char*>(
        base::AlignedAlloc(output_size_, PaintOpBuffer::PaintOpAlign)));
  }

  bool IsTypeSupported() {
    // DrawRecordOps must be flattened and are not currently serialized.
    // All other types must push non-zero amounts of ops in PushTestOps.
    return GetParamType() != PaintOpType::DrawRecord;
  }

 protected:
  std::unique_ptr<char, base::AlignedFreeDeleter> output_;
  size_t output_size_ = 0u;
  PaintOpBuffer buffer_;
};

INSTANTIATE_TEST_CASE_P(
    P,
    PaintOpSerializationTest,
    ::testing::Range(static_cast<uint8_t>(0),
                     static_cast<uint8_t>(PaintOpType::LastPaintOpType)));

// Test serializing and then deserializing all test ops.  They should all
// write successfully and be identical to the original ops in the buffer.
TEST_P(PaintOpSerializationTest, SmokeTest) {
  if (!IsTypeSupported())
    return;

  PushTestOps(GetParamType());

  ResizeOutputBuffer();

  SimpleSerializer serializer(output_.get(), output_size_);
  serializer.Serialize(buffer_);

  // Expect all ops to write more than 0 bytes.
  for (size_t i = 0; i < buffer_.size(); ++i) {
    SCOPED_TRACE(base::StringPrintf(
        "%s #%zd", PaintOpTypeToString(GetParamType()).c_str(), i));
    EXPECT_GT(serializer.bytes_written()[i], 0u);
  }

  PaintOpBuffer::Iterator iter(&buffer_);
  size_t i = 0;
  for (auto* base_written :
       DeserializerIterator(output_.get(), serializer.TotalBytesWritten())) {
    SCOPED_TRACE(base::StringPrintf(
        "%s #%zu", PaintOpTypeToString(GetParamType()).c_str(), i));
    PaintOpUtil::ExpectOpsEqual(*iter, base_written);
    ++iter;
    ++i;
  }

  EXPECT_EQ(buffer_.size(), i);
}

// Verify for all test ops that serializing into a smaller size aborts
// correctly and doesn't write anything.
TEST_P(PaintOpSerializationTest, SerializationFailures) {
  if (!IsTypeSupported())
    return;

  PushTestOps(GetParamType());

  ResizeOutputBuffer();

  SimpleSerializer serializer(output_.get(), output_size_);
  serializer.Serialize(buffer_);
  std::vector<size_t> bytes_written = serializer.bytes_written();

  PaintOp::SerializeOptions options;

  size_t op_idx = 0;
  for (PaintOpBuffer::Iterator iter(&buffer_); iter; ++iter, ++op_idx) {
    SCOPED_TRACE(base::StringPrintf(
        "%s #%zu", PaintOpTypeToString(GetParamType()).c_str(), op_idx));
    size_t expected_bytes = bytes_written[op_idx];
    EXPECT_GT(expected_bytes, 0u);

    // Attempt to write op into a buffer of size |i|, and only expect
    // it to succeed if the buffer is large enough.
    for (size_t i = 0; i < bytes_written[op_idx] + 2; ++i) {
      size_t written_bytes = iter->Serialize(output_.get(), i, options);
      if (i >= expected_bytes) {
        EXPECT_EQ(expected_bytes, written_bytes) << "i: " << i;
      } else {
        EXPECT_EQ(0u, written_bytes) << "i: " << i;
      }
    }
  }
}

// Verify that deserializing test ops from too small buffers aborts
// correctly, in case the deserialized data is lying about how big it is.
TEST_P(PaintOpSerializationTest, DeserializationFailures) {
  if (!IsTypeSupported())
    return;

  PushTestOps(GetParamType());

  ResizeOutputBuffer();

  SimpleSerializer serializer(output_.get(), output_size_);
  serializer.Serialize(buffer_);

  char* current = static_cast<char*>(output_.get());

  static constexpr size_t kAlign = PaintOpBuffer::PaintOpAlign;
  static constexpr size_t kOutputOpSize = kBufferBytesPerOp;
  std::unique_ptr<char, base::AlignedFreeDeleter> deserialize_buffer_(
      static_cast<char*>(base::AlignedAlloc(kOutputOpSize, kAlign)));

  size_t op_idx = 0;
  for (PaintOpBuffer::Iterator iter(&buffer_); iter; ++iter, ++op_idx) {
    PaintOp* serialized = reinterpret_cast<PaintOp*>(current);
    uint32_t skip = serialized->skip;

    // Read from buffers of various sizes to make sure that having a serialized
    // op size that is larger than the input buffer provided causes a
    // deserialization failure to return nullptr.  Also test a few valid sizes
    // larger than read size.
    for (size_t read_size = 0; read_size < skip + kAlign * 2 + 2; ++read_size) {
      SCOPED_TRACE(base::StringPrintf(
          "%s #%zd, read_size: %zu",
          PaintOpTypeToString(GetParamType()).c_str(), op_idx, read_size));
      // Because PaintOp::Deserialize early outs when the input size is < skip
      // deliberately lie about the skip.  This op tooooootally fits.
      // This will verify that individual op deserializing code behaves
      // properly when presented with invalid offsets.
      serialized->skip = read_size;
      PaintOp* written = PaintOp::Deserialize(
          current, read_size, deserialize_buffer_.get(), kOutputOpSize);

      // Skips are only valid if they are aligned.
      if (read_size >= skip && read_size % kAlign == 0) {
        ASSERT_NE(nullptr, written);
        ASSERT_LE(written->skip, kOutputOpSize);
        EXPECT_EQ(GetParamType(), written->GetType());
      } else {
        EXPECT_EQ(nullptr, written);
      }

      if (written)
        written->DestroyThis();
    }

    current += skip;
  }
}

// Test generic PaintOp deserializing failure cases.
TEST(PaintOpBufferTest, PaintOpDeserialize) {
  static constexpr size_t kSize = sizeof(LargestPaintOp) + 100;
  static constexpr size_t kAlign = PaintOpBuffer::PaintOpAlign;
  std::unique_ptr<char, base::AlignedFreeDeleter> input_(
      static_cast<char*>(base::AlignedAlloc(kSize, kAlign)));
  std::unique_ptr<char, base::AlignedFreeDeleter> output_(
      static_cast<char*>(base::AlignedAlloc(kSize, kAlign)));

  PaintOpBuffer buffer;
  buffer.push<DrawColorOp>(SK_ColorMAGENTA, SkBlendMode::kSrc);

  PaintOpBuffer::Iterator iter(&buffer);
  PaintOp* op = *iter;
  ASSERT_TRUE(op);

  PaintOp::SerializeOptions options;
  size_t bytes_written = op->Serialize(input_.get(), kSize, options);
  ASSERT_GT(bytes_written, 0u);

  // can deserialize from exactly the right size
  PaintOp* success =
      PaintOp::Deserialize(input_.get(), bytes_written, output_.get(), kSize);
  ASSERT_TRUE(success);
  success->DestroyThis();

  // fail to deserialize if skip goes past input size
  // (the DeserializationFailures test above tests if the skip is lying)
  for (size_t i = 0; i < bytes_written - 1; ++i)
    EXPECT_FALSE(PaintOp::Deserialize(input_.get(), i, output_.get(), kSize));

  // unaligned skips fail to deserialize
  PaintOp* serialized = reinterpret_cast<PaintOp*>(input_.get());
  EXPECT_EQ(0u, serialized->skip % kAlign);
  serialized->skip -= 1;
  EXPECT_FALSE(
      PaintOp::Deserialize(input_.get(), bytes_written, output_.get(), kSize));
  serialized->skip += 1;

  // bogus types fail to deserialize
  serialized->type = static_cast<uint8_t>(PaintOpType::LastPaintOpType) + 1;
  EXPECT_FALSE(
      PaintOp::Deserialize(input_.get(), bytes_written, output_.get(), kSize));
}

// Test that deserializing invalid SkClipOp enums fails silently.
// Skia release asserts on this in several places so these are not safe
// to pass through to the SkCanvas API.
TEST(PaintOpBufferTest, ValidateSkClip) {
  size_t buffer_size = kBufferBytesPerOp;
  std::unique_ptr<char, base::AlignedFreeDeleter> serialized(static_cast<char*>(
      base::AlignedAlloc(buffer_size, PaintOpBuffer::PaintOpAlign)));
  std::unique_ptr<char, base::AlignedFreeDeleter> deserialized(
      static_cast<char*>(
          base::AlignedAlloc(buffer_size, PaintOpBuffer::PaintOpAlign)));

  PaintOpBuffer buffer;

  // Successful first op.
  SkPath path;
  buffer.push<ClipPathOp>(path, SkClipOp::kMax_EnumValue, true);

  // Bad other ops.
  SkClipOp bad_clip = static_cast<SkClipOp>(
      static_cast<uint32_t>(SkClipOp::kMax_EnumValue) + 1);

  const auto& test_rects = PaintOpUtil::TestRects();
  const auto& test_rrects = PaintOpUtil::TestRRects();

  buffer.push<ClipPathOp>(path, bad_clip, true);
  buffer.push<ClipRectOp>(test_rects[0], bad_clip, true);
  buffer.push<ClipRRectOp>(test_rrects[0], bad_clip, false);

  SkClipOp bad_clip_max = static_cast<SkClipOp>(~static_cast<uint32_t>(0));
  buffer.push<ClipRectOp>(test_rects[1], bad_clip_max, false);

  PaintOp::SerializeOptions options;

  int op_idx = 0;
  for (PaintOpBuffer::Iterator iter(&buffer); iter; ++iter) {
    const PaintOp* op = *iter;
    size_t bytes_written =
        op->Serialize(serialized.get(), buffer_size, options);
    ASSERT_GT(bytes_written, 0u);
    PaintOp* written = PaintOp::Deserialize(serialized.get(), bytes_written,
                                            deserialized.get(), buffer_size);
    // First op should succeed.  Other ops with bad enums should
    // serialize correctly but fail to deserialize due to the bad
    // SkClipOp enum.
    if (!op_idx) {
      EXPECT_TRUE(written) << "op: " << op_idx;
      written->DestroyThis();
    } else {
      EXPECT_FALSE(written) << "op: " << op_idx;
    }

    ++op_idx;
  }
}

TEST(PaintOpBufferTest, ValidateSkBlendMode) {
  size_t buffer_size = kBufferBytesPerOp;
  std::unique_ptr<char, base::AlignedFreeDeleter> serialized(static_cast<char*>(
      base::AlignedAlloc(buffer_size, PaintOpBuffer::PaintOpAlign)));
  std::unique_ptr<char, base::AlignedFreeDeleter> deserialized(
      static_cast<char*>(
          base::AlignedAlloc(buffer_size, PaintOpBuffer::PaintOpAlign)));

  PaintOpBuffer buffer;

  const auto& test_flags = PaintOpUtil::TestFlags();
  const auto& test_rects = PaintOpUtil::TestRects();

  // Successful first two ops.
  buffer.push<DrawColorOp>(SK_ColorMAGENTA, SkBlendMode::kDstIn);
  PaintFlags good_flags = test_flags[0];
  good_flags.setBlendMode(SkBlendMode::kColorBurn);
  buffer.push<DrawRectOp>(test_rects[0], good_flags);

  // Modes that are not supported by drawColor or SkPaint.
  SkBlendMode bad_modes_for_draw_color[] = {
      SkBlendMode::kOverlay,
      SkBlendMode::kDarken,
      SkBlendMode::kLighten,
      SkBlendMode::kColorDodge,
      SkBlendMode::kColorBurn,
      SkBlendMode::kHardLight,
      SkBlendMode::kSoftLight,
      SkBlendMode::kDifference,
      SkBlendMode::kExclusion,
      SkBlendMode::kMultiply,
      SkBlendMode::kHue,
      SkBlendMode::kSaturation,
      SkBlendMode::kColor,
      SkBlendMode::kLuminosity,
      static_cast<SkBlendMode>(static_cast<uint32_t>(SkBlendMode::kLastMode) +
                               1),
      static_cast<SkBlendMode>(static_cast<uint32_t>(~0)),
  };

  SkBlendMode bad_modes_for_flags[] = {
      static_cast<SkBlendMode>(static_cast<uint32_t>(SkBlendMode::kLastMode) +
                               1),
      static_cast<SkBlendMode>(static_cast<uint32_t>(~0)),
  };

  for (size_t i = 0; i < arraysize(bad_modes_for_draw_color); ++i) {
    buffer.push<DrawColorOp>(SK_ColorMAGENTA, bad_modes_for_draw_color[i]);
  }

  for (size_t i = 0; i < arraysize(bad_modes_for_flags); ++i) {
    PaintFlags flags = test_flags[i % test_flags.size()];
    flags.setBlendMode(bad_modes_for_flags[i]);
    buffer.push<DrawRectOp>(test_rects[i % test_rects.size()], flags);
  }

  PaintOp::SerializeOptions options;

  int op_idx = 0;
  for (PaintOpBuffer::Iterator iter(&buffer); iter; ++iter) {
    const PaintOp* op = *iter;
    size_t bytes_written =
        op->Serialize(serialized.get(), buffer_size, options);
    ASSERT_GT(bytes_written, 0u);
    PaintOp* written = PaintOp::Deserialize(serialized.get(), bytes_written,
                                            deserialized.get(), buffer_size);
    // First two ops should succeed.  Other ops with bad enums should
    // serialize correctly but fail to deserialize due to the bad
    // SkBlendMode enum.
    if (op_idx < 2) {
      EXPECT_TRUE(written) << "op: " << op_idx;
      written->DestroyThis();
    } else {
      EXPECT_FALSE(written) << "op: " << op_idx;
    }

    ++op_idx;
  }
}

}  // namespace cc

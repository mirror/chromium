// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_OP_BUFFER_SERIALIZER_H_
#define CC_PAINT_PAINT_OP_BUFFER_SERIALIZER_H_

#include "cc/paint/paint_op_buffer.h"

#include "third_party/skia/include/utils/SkNoDrawCanvas.h"

namespace cc {

template <typename T>
class CC_PAINT_EXPORT PaintOpBufferSerializer {
 public:
  PaintOpBufferSerializer(T* op_serializer, ImageProvider* image_provider)
      : op_serializer_(op_serializer),
        canvas_(100, 100),
        image_provider_(image_provider) {
    DCHECK(op_serializer_);
  }
  ~PaintOpBufferSerializer() = default;

  void Serialize(const PaintOpBuffer* buffer,
                 const std::vector<size_t>* offsets,
                 const PaintOpBuffer* preamble) {
    PaintOp::SerializeOptions options(image_provider_, &canvas_,
                                      canvas_.getTotalMatrix());
    PlaybackParams params(image_provider_, canvas_.getTotalMatrix());

    int save_count = canvas_.getSaveCount();
    SerializePreamble(preamble, options, params);
    Save(options, params);
    SerializeBuffer(buffer, offsets);
    RestoreToCount(save_count, options, params);
  }

  size_t total_bytes() const { return total_bytes_; }
  bool valid() const { return valid_; }

 private:
  void SerializePreamble(const PaintOpBuffer* preamble,
                         const PaintOp::SerializeOptions& options,
                         const PlaybackParams& params) {
    if (!preamble)
      return;
    Save(options, params);
    for (const auto* op : PaintOpBuffer::Iterator(preamble)) {
      if (!SerializeOp(op, options, params))
        return;
    }
  }

  void SerializeBuffer(const PaintOpBuffer* buffer,
                       const std::vector<size_t>* offsets) {
    if (!valid_)
      return;

    PaintOp::SerializeOptions options(image_provider_, &canvas_,
                                      canvas_.getTotalMatrix());
    PlaybackParams params(image_provider_, canvas_.getTotalMatrix());

    // TODO(khushalsagar): Use PlaybackFoldingIterator so we do alpha folding
    // at serialization.
    for (PaintOpBuffer::CompositeIterator iter(buffer, offsets); iter; ++iter) {
      const PaintOp* op = *iter;

      // Skip ops outside the current clip if they have images. This saves
      // performing an unnecessary expensive decode.
      const bool skip_op = PaintOp::OpHasDiscardableImages(op) &&
                           PaintOp::QuickRejectDraw(op, &canvas_);
      if (skip_op)
        continue;

      if (op->GetType() != PaintOpType::DrawRecord) {
        if (!SerializeOp(op, options, params))
          return;
        continue;
      }

      int save_count = canvas_.getSaveCount();
      Save(options, params);
      SerializeBuffer(static_cast<const DrawRecordOp*>(op)->record.get(),
                      nullptr);
      RestoreToCount(save_count, options, params);
    }
  }

 private:
  bool SerializeOp(const PaintOp* op,
                   const PaintOp::SerializeOptions& options,
                   const PlaybackParams& params) {
    size_t bytes = op_serializer_->Serialize(op, options);
    if (!bytes) {
      valid_ = false;
      return false;
    }

    DCHECK_GE(bytes, 4u);
    DCHECK_EQ(bytes % PaintOpBuffer::PaintOpAlign, 0u);

    total_bytes_ += bytes;
    // Only pass state-changing operations to the canvas.
    if (!op->IsDrawOp())
      op->Raster(&canvas_, params);
    return true;
  }

  void Save(const PaintOp::SerializeOptions& options,
            const PlaybackParams& params) {
    SaveOp save_op;
    save_op.type = static_cast<uint8_t>(PaintOpType::Save);
    SerializeOp(&save_op, options, params);
  }

  void RestoreToCount(int count,
                      const PaintOp::SerializeOptions& options,
                      const PlaybackParams& params) {
    RestoreOp restore_op;
    restore_op.type = static_cast<uint8_t>(PaintOpType::Restore);
    while (canvas_.getSaveCount() > count)
      SerializeOp(&restore_op, options, params);
  }

 private:
  T* op_serializer_;
  SkNoDrawCanvas canvas_;
  ImageProvider* image_provider_;
  size_t total_bytes_;
  bool valid_ = true;
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_OP_BUFFER_SERIALIZER_H_

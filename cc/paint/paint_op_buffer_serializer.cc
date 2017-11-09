// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_op_buffer_serializer.h"

#include "base/bind.h"

namespace cc {

PaintOpBufferSerializer::PaintOpBufferSerializer(
    SerializeCallback serialize_cb, ImageProvider* image_provider)
    : serialize_cb_(std::move(serialize_cb)),
      canvas_(100, 100),
      image_provider_(image_provider) {
  DCHECK(serialize_cb_);
}

PaintOpBufferSerializer::~PaintOpBufferSerializer() = default;

void PaintOpBufferSerializer::Serialize(const PaintOpBuffer* buffer,
               const std::vector<size_t>* offsets,
               const PaintOpBuffer* preamble) {
  PaintOp::SerializeOptions options(image_provider_, &canvas_,
                                    canvas_.getTotalMatrix());
  PlaybackParams params(image_provider_, canvas_.getTotalMatrix());

  int save_count = canvas_.getSaveCount();
  Save(options, params);
  SerializePreamble(preamble, options, params);
  SerializeBuffer(buffer, offsets);
  RestoreToCount(save_count, options, params);
}

void PaintOpBufferSerializer::SerializePreamble(const PaintOpBuffer* preamble,
                       const PaintOp::SerializeOptions& options,
                       const PlaybackParams& params) {
  if (!preamble)
    return;
  for (const auto* op : PaintOpBuffer::Iterator(preamble)) {
    if (!SerializeOp(op, options, params))
      return;
  }
}

void PaintOpBufferSerializer::SerializeBuffer(const PaintOpBuffer* buffer,
                     const std::vector<size_t>* offsets) {
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

bool PaintOpBufferSerializer::SerializeOp(const PaintOp* op,
                 const PaintOp::SerializeOptions& options,
                 const PlaybackParams& params) {
  if (!valid_)
    return false;

  size_t bytes = serialize_cb_.Run(op, options);
  if (!bytes) {
    valid_ = false;
    return false;
  }

  DCHECK_GE(bytes, 4u);
  DCHECK_EQ(bytes % PaintOpBuffer::PaintOpAlign, 0u);

  // Only pass state-changing operations to the canvas.
  if (!op->IsDrawOp())
    op->Raster(&canvas_, params);
  return true;
}

void PaintOpBufferSerializer::Save(const PaintOp::SerializeOptions& options,
          const PlaybackParams& params) {
  SaveOp save_op;
  save_op.type = static_cast<uint8_t>(PaintOpType::Save);
  SerializeOp(&save_op, options, params);
}

void PaintOpBufferSerializer::RestoreToCount(int count,
                    const PaintOp::SerializeOptions& options,
                    const PlaybackParams& params) {
  RestoreOp restore_op;
  restore_op.type = static_cast<uint8_t>(PaintOpType::Restore);
  while (canvas_.getSaveCount() > count) {
    if (!SerializeOp(&restore_op, options, params))
      return;
  }
}

SimpleBufferSerializer::SimpleBufferSerializer(void* memory, size_t size,
                                               ImageProvider* image_provider)
  : PaintOpBufferSerializer(
      base::Bind(&SimpleBufferSerializer::SerializeToMemory,
                 base::Unretained(this)), image_provider),
      memory_(memory), total_(size) {}

SimpleBufferSerializer::~SimpleBufferSerializer() = default;

size_t SimpleBufferSerializer::SerializeToMemory(
    const PaintOp* op, const PaintOp::SerializeOptions& options) {
  size_t bytes = op->Serialize(static_cast<char*>(memory_) + written_,
                               total_ - written_, options);
  if (!bytes)
    return 0u;
  written_ += bytes;
  return bytes;
}

}  // namespace cc

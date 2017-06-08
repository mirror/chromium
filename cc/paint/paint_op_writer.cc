// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_op_writer.h"

#include "cc/paint/paint_flags.h"
#include "third_party/skia/include/core/SkTextBlob.h"
#include "third_party/skia/include/core/SkWriteBuffer.h"

namespace cc {

template <typename T>
void PaintOpWriter::WriteSimple(const T& val) {
  static_assert(base::is_trivially_copyable<T>::value, "");
  if (sizeof(T) > remaining_bytes_)
    valid_ = false;
  if (!valid_)
    return;

  reinterpret_cast<T*>(memory_)[0] = val;

  memory_ += sizeof(T);
  remaining_bytes_ -= sizeof(T);
}

template <typename T>
void PaintOpWriter::WriteFlattened(const T& val) {
  SkBinaryWriteBuffer write_buffer(memory_, remaining_bytes_,
                                   SkBinaryWriteBuffer::kCrossProcess_Flag);

  val.flatten(write_buffer);

  // Skia writers allocate heap storage if the initial storage is not
  // sufficient.  In PaintOpWriter's case, just throw this work away.
  if (write_buffer.bytesWritten() > remaining_bytes_)
    valid_ = false;
}

void PaintOpWriter::Write(size_t data) {
  WriteSimple(data);
}

void PaintOpWriter::Write(SkScalar data) {
  WriteSimple(data);
}

void PaintOpWriter::Write(uint8_t data) {
  WriteSimple(data);
}

void PaintOpWriter::Write(const SkRect& rect) {
  WriteSimple(rect);
}

void PaintOpWriter::Write(const SkIRect& rect) {
  WriteSimple(rect);
}

void PaintOpWriter::Write(const SkRRect& rect) {
  WriteSimple(rect);
}

void PaintOpWriter::Write(const SkPath& path) {
  size_t bytes = path.writeToMemory(nullptr);
  if (bytes > remaining_bytes_)
    valid_ = false;
  if (!valid_)
    return;

  path.writeToMemory(memory_);
  memory_ += bytes;
  remaining_bytes_ -= bytes;
}

void PaintOpWriter::Write(const PaintFlags& flags) {
  WriteFlattened(flags.paint_);
}

void PaintOpWriter::Write(const PaintImage& image) {
  // TODO(enne)
}

void PaintOpWriter::Write(const sk_sp<SkData>& data) {
  WriteData(data->size(), data->data());
}

void PaintOpWriter::Write(const sk_sp<SkTextBlob>& blob) {
  WriteFlattened(*blob);
}

void PaintOpWriter::WriteData(size_t bytes, const void* input) {
  Write(bytes);

  if (bytes > remaining_bytes_)
    valid_ = false;
  if (!valid_)
    return;

  memcpy(memory_, input, bytes);
  memory_ += bytes;
  remaining_bytes_ -= bytes;
}

void PaintOpWriter::WriteArray(size_t count, const SkPoint* input) {
  size_t bytes = sizeof(SkPoint) * count;
  WriteData(bytes, input);
}

}  // namespace cc

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "cc/paint/paint_op_buffer.h"

// paint_op_buffer_eq_fuzzer deserializes and reserializes paint ops to
// make sure that this does not modify or incorrectly serialize them.
// This is intended to be a fuzzing correctness test.
//
// Compare this to paint_op_buffer_fuzzer which makes sure that deserializing
// ops and rasterizing those ops is safe.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  const size_t kMaxSerializedSize = 1000000;

  // TODO(enne): add an image provider here once deserializing supports that.
  cc::PaintOp::SerializeOptions serialize_options;
  cc::PaintOp::DeserializeOptions deserialize_options;

  // Need 4 bytes to be able to read the type/skip.
  if (size < 4)
    return 0;

  const cc::PaintOp* serialized = reinterpret_cast<const cc::PaintOp*>(data);
  if (serialized->skip > kMaxSerializedSize)
    return 0;

  // If the op has a skip that runs off the end of the input, then ignore.
  if (serialized->skip > size)
    return 0;

  std::unique_ptr<char, base::AlignedFreeDeleter> deserialized(
      static_cast<char*>(base::AlignedAlloc(sizeof(cc::LargestPaintOp),
                                            cc::PaintOpBuffer::PaintOpAlign)));
  size_t bytes_read = 0;
  cc::PaintOp* deserialized_op = cc::PaintOp::Deserialize(
      data, size, deserialized.get(), sizeof(cc::LargestPaintOp), &bytes_read,
      deserialize_options);

  // Failed to deserialize, so abort.
  if (!deserialized_op)
    return 0;

  // TODO(enne): Text blobs sometimes are "valid" but null, which is an
  // interim state as text blob serialization has not been completed.
  // It needs to be valid in practice (so that web pages don't crash) but
  // should be invalid for the fuzzer (as this cannot reserialize).
  // See also: TODO in PaintOpReader::Read(scoped_refptr<PaintTextBlob>*)
  if (deserialized_op->GetType() == cc::PaintOpType::DrawTextBlob) {
    auto* draw_text_op = static_cast<cc::DrawTextBlobOp*>(deserialized_op);
    if (!draw_text_op->blob->ToSkTextBlob()) {
      deserialized_op->DestroyThis();
      return 0;
    }
  }

  // Reserialize and compare equality.
  std::unique_ptr<char, base::AlignedFreeDeleter> reserialized(
      static_cast<char*>(base::AlignedAlloc(serialized->skip,
                                            cc::PaintOpBuffer::PaintOpAlign)));
  size_t written_bytes = deserialized_op->Serialize(
      reserialized.get(), serialized->skip, serialize_options);
  deserialized_op->DestroyThis();

  CHECK_GT(written_bytes, 0u);
  CHECK_LE(written_bytes, serialized->skip);
  CHECK_EQ(memcmp(data, reserialized.get(), serialized->skip), 0);

  return 0;
}

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UnpackedSerializedScriptValue_h
#define UnpackedSerializedScriptValue_h

#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "core/CoreExport.h"
#include "platform/heap/GarbageCollected.h"
#include "platform/heap/Handle.h"
#include "platform/heap/HeapAllocator.h"
#include "platform/wtf/RefPtr.h"
#include "platform/wtf/Vector.h"

namespace blink {

class DOMArrayBufferBase;
class ImageBitmap;

// TODO(jbroman): Describe this!
class CORE_EXPORT UnpackedSerializedScriptValue
    : public GarbageCollectedFinalized<UnpackedSerializedScriptValue> {
 public:
  ~UnpackedSerializedScriptValue();

  DECLARE_TRACE();

  SerializedScriptValue* Value() { return value_.Get(); }
  const SerializedScriptValue* Value() const { return value_.Get(); }

  const HeapVector<Member<DOMArrayBufferBase>>& ArrayBuffers() const {
    return array_buffers_;
  }
  const HeapVector<Member<ImageBitmap>>& ImageBitmaps() const {
    return image_bitmaps_;
  }

  using DeserializeOptions = SerializedScriptValue::DeserializeOptions;
  v8::Local<v8::Value> Deserialize(
      v8::Isolate*,
      const DeserializeOptions& = DeserializeOptions());

 private:
  // Private so that callers use SerializedScriptValue::Unpack.
  explicit UnpackedSerializedScriptValue(RefPtr<SerializedScriptValue>);

  // The underlying serialized data.
  RefPtr<SerializedScriptValue> value_;

  // These replace their corresponding members in SerializedScriptValue, once
  // set. Once the value is being deserialized, objects will be materialized
  // here.
  HeapVector<Member<DOMArrayBufferBase>> array_buffers_;
  HeapVector<Member<ImageBitmap>> image_bitmaps_;

  friend class SerializedScriptValue;
};

}  // namespace blink

#endif  // UnpackedSerializedScriptValue_h

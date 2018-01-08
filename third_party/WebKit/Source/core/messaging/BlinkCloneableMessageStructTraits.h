// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BlinkCloneableMessageStructTraits_h
#define BlinkCloneableMessageStructTraits_h

#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "core/messaging/BlinkCloneableMessage.h"
#include "mojo/public/cpp/bindings/array_traits_wtf_vector.h"
#include "mojo/public/cpp/bindings/string_traits_wtf.h"
#include "third_party/WebKit/Source/platform/wtf/Vector.h"
#include "third_party/WebKit/Source/platform/wtf/typed_arrays/ArrayBufferContents.h"
#include "third_party/WebKit/common/message_port/message_port.mojom-blink.h"

namespace mojo {

template <>
struct StructTraits<blink::mojom::blink::CloneableMessage::DataView,
                    blink::BlinkCloneableMessage> {
  static base::span<const uint8_t> encoded_message(
      blink::BlinkCloneableMessage& input) {
    return input.message->GetWireData();
  }

  static Vector<blink::mojom::blink::SerializedBlobPtr> blobs(
      blink::BlinkCloneableMessage& input);
  static Vector<WTF::ArrayBufferContents>& arrayBufferContentsArray(
      blink::BlinkCloneableMessage& input);

  static bool Read(blink::mojom::blink::CloneableMessage::DataView,
                   blink::BlinkCloneableMessage* out);
};

template <>
class StructTraits<blink::mojom::blink::SerializedArrayBufferContents::DataView,
                   WTF::ArrayBufferContents> {
 public:
  static base::span<const uint8_t> contents(WTF::ArrayBufferContents& abc) {
    uint8_t* start = static_cast<uint8_t*>(abc.Data());
    const size_t s = abc.SizeInBytes();
    return base::span<uint8_t>(start, s);
  }
  static bool Read(
      blink::mojom::blink::SerializedArrayBufferContents::DataView,
      WTF::ArrayBufferContents* out);
};

}  // namespace mojo

#endif  // BlinkCloneableMessageStructTraits_h

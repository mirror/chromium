// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BlinkTransferableMessageStructTraits_h
#define BlinkTransferableMessageStructTraits_h

#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "core/imagebitmap/ImageBitmap.h"
#include "core/messaging/BlinkCloneableMessageStructTraits.h"
#include "core/messaging/BlinkTransferableMessage.h"
#include "mojo/public/cpp/bindings/array_traits_wtf_vector.h"
#include "third_party/WebKit/Source/platform/graphics/Image.h"
#include "third_party/WebKit/Source/platform/wtf/typed_arrays/ArrayBufferContents.h"
#include "third_party/WebKit/common/message_port/message_port.mojom-blink.h"
#include "third_party/WebKit/common/message_port/message_port_channel.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace mojo {

template <>
struct StructTraits<blink::mojom::blink::TransferableMessage::DataView,
                    blink::BlinkTransferableMessage> {
  static blink::BlinkCloneableMessage& message(
      blink::BlinkTransferableMessage& input) {
    return input;
  }

  static Vector<mojo::ScopedMessagePipeHandle> ports(
      blink::BlinkTransferableMessage& input) {
    Vector<mojo::ScopedMessagePipeHandle> result;
    result.ReserveInitialCapacity(input.ports.size());
    for (const auto& port : input.ports)
      result.push_back(port.ReleaseHandle());
    return result;
  }

  static Vector<WTF::ArrayBufferContents>& arrayBufferContentsArray(
      blink::BlinkCloneableMessage& input) {
    return input.message->GetArrayBufferContentsArray();
  }

  static Vector<SkBitmap> imageBitmapContentsArray(
      blink::BlinkCloneableMessage& input);

  static bool Read(blink::mojom::blink::TransferableMessage::DataView,
                   blink::BlinkTransferableMessage* out);
};

template <>
class StructTraits<blink::mojom::blink::SerializedArrayBufferContents::DataView,
                   WTF::ArrayBufferContents> {
 public:
  static std::vector<uint8_t> contents(WTF::ArrayBufferContents& abc) {
    return std::vector<uint8_t>(
        static_cast<uint8_t*>(abc.Data()),
        static_cast<uint8_t*>(abc.Data()) + abc.DataLength());
  }
  static bool Read(blink::mojom::blink::SerializedArrayBufferContents::DataView,
                   WTF::ArrayBufferContents* out);
};

}  // namespace mojo

#endif  // BlinkTransferableMessageStructTraits_h

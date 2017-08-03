// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MessagePortMessageStructTraits_h
#define MessagePortMessageStructTraits_h

#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "core/dom/MessagePortMessage.h"
#include "third_party/WebKit/common/message_port/message_port.h"
#include "third_party/WebKit/common/message_port/message_port.mojom-blink.h"

namespace mojo {

template <>
struct StructTraits<blink_common::mojom::blink::MessagePortMessage::DataView,
                    blink::BlinkMessagePortMessage> {
  static ConstCArray<uint8_t> encoded_message(
      blink::BlinkMessagePortMessage& input) {
    StringView wire_data = input.message->GetWireData();
    return ConstCArray<uint8_t>(
        wire_data.length(),
        reinterpret_cast<const uint8_t*>(wire_data.Characters8()));
  }

  static std::vector<mojo::ScopedMessagePipeHandle> ports(
      blink::BlinkMessagePortMessage& input) {
    return blink_common::MessagePort::ReleaseHandles(input.ports);
  }

  static bool Read(blink_common::mojom::blink::MessagePortMessage::DataView,
                   blink::BlinkMessagePortMessage* out);
};

}  // namespace mojo

#endif  // MessagePortMessageStructTraits_h

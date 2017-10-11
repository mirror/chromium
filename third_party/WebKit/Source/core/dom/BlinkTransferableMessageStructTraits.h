// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BlinkTransferableMessageStructTraits_h
#define BlinkTransferableMessageStructTraits_h

#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "core/dom/BlinkCloneableMessageStructTraits.h"
#include "core/dom/BlinkTransferableMessage.h"
#include "third_party/WebKit/common/message_port/message_port.mojom-blink.h"
#include "third_party/WebKit/common/message_port/message_port_channel.h"

namespace mojo {

template <>
struct StructTraits<blink::mojom::blink::TransferableMessage::DataView,
                    blink::BlinkTransferableMessage> {
  static blink::BlinkCloneableMessage& message(
      blink::BlinkTransferableMessage& input) {
    return input;
  }

  static std::vector<mojo::ScopedMessagePipeHandle> ports(
      blink::BlinkTransferableMessage& input) {
    std::vector<blink::MessagePortChannel> channels(input.ports.begin(),
                                                    input.ports.end());
    return blink::MessagePortChannel::ReleaseHandles(std::move(channels));
  }

  static bool Read(blink::mojom::blink::TransferableMessage::DataView,
                   blink::BlinkTransferableMessage* out);
};

}  // namespace mojo

#endif  // BlinkTransferableMessageStructTraits_h

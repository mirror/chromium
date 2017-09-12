// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_COMMON_MESSAGE_PORT_MESSAGE_PORT_MESSAGE_STRUCT_TRAITS_H_
#define THIRD_PARTY_WEBKIT_COMMON_MESSAGE_PORT_MESSAGE_PORT_MESSAGE_STRUCT_TRAITS_H_

#include "third_party/WebKit/common/message_port/message_port.mojom.h"
#include "third_party/WebKit/common/message_port/message_port_message.h"

namespace mojo {

template <>
struct StructTraits<blink_common::mojom::MessagePortMessage::DataView,
                    blink_common::MessagePortMessage> {
  static base::span<const uint8_t> encoded_message(
      blink_common::MessagePortMessage& input) {
    return input.encoded_message;
  }

  static std::vector<mojo::ScopedMessagePipeHandle>& ports(
      blink_common::MessagePortMessage& input) {
    return input.ports;
  }

  static bool Read(blink_common::mojom::MessagePortMessage::DataView data,
                   blink_common::MessagePortMessage* out);
};

}  // namespace mojo

#endif  // THIRD_PARTY_WEBKIT_COMMON_MESSAGE_PORT_MESSAGE_PORT_MESSAGE_STRUCT_TRAITS_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_COMMON_MESSAGE_PORT_MESSAGE_PORT_MESSAGE_STRUCT_TRAITS_H_
#define THIRD_PARTY_WEBKIT_COMMON_MESSAGE_PORT_MESSAGE_PORT_MESSAGE_STRUCT_TRAITS_H_

#include "third_party/WebKit/common/message_port/message_port.mojom.h"

namespace blink_common {

// This struct represents messages as they are posted over a message port. This
// type can be serialized as a content::mojom::MessagePortMessage struct.
struct MessagePortMessage {
  MessagePortMessage();
  ~MessagePortMessage();

  // To reduce copies when serializing |encoded_message| does not have to point
  // to |owned_encoded_message|. The serialization code completely ignores the
  // |owned_encoded_message| and just serializes whatever |encoded_message|
  // points to. When deserializing |owned_encoded_message| is set to the data
  // and |encoded_message| is set to point to |owned_encoded_message|.
  mojo::ConstCArray<uint8_t> encoded_message;
  std::vector<uint8_t> owned_encoded_message;

  // Any ports being transfered as part of this message.
  std::vector<mojo::ScopedMessagePipeHandle> ports;
};

}  // namespace blink_common

namespace mojo {

template <>
struct StructTraits<blink_common::mojom::MessagePortMessage::DataView,
                    blink_common::MessagePortMessage> {
  static ConstCArray<uint8_t> encoded_message(
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

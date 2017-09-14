// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_COMMON_MESSAGE_PORT_MESSAGE_PORT_MESSAGE_H_
#define THIRD_PARTY_WEBKIT_COMMON_MESSAGE_PORT_MESSAGE_PORT_MESSAGE_H_

#include <vector>

#include "base/containers/span.h"
#include "storage/public/interfaces/blobs.mojom.h"
#include "third_party/WebKit/common/common_export.h"
#include "third_party/WebKit/common/message_port/message_port_channel.h"

namespace blink {

// This struct represents messages as they are posted over a message port. This
// type can be serialized as a content::mojom::MessagePortMessage struct.
struct BLINK_COMMON_EXPORT MessagePortMessage {
  MessagePortMessage();
  MessagePortMessage(MessagePortMessage&&);
  MessagePortMessage& operator=(MessagePortMessage&&);
  ~MessagePortMessage();

  // To reduce copies when serializing |encoded_message| does not have to point
  // to |owned_encoded_message|. The serialization code completely ignores the
  // |owned_encoded_message| and just serializes whatever |encoded_message|
  // points to. When deserializing |owned_encoded_message| is set to the data
  // and |encoded_message| is set to point to |owned_encoded_message|.
  base::span<const uint8_t> encoded_message;
  std::vector<uint8_t> owned_encoded_message;

  // Any ports being transfered as part of this message.
  std::vector<MessagePortChannel> ports;

  // Blob handles for any blobs being send in this message.
  std::vector<storage::mojom::SerializedBlobPtr> blobs;
};

}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_COMMON_MESSAGE_PORT_MESSAGE_PORT_MESSAGE_H_

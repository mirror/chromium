// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BlinkTransferableMessage_h
#define BlinkTransferableMessage_h

#include "base/macros.h"
#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "core/CoreExport.h"
#include "core/messaging/BlinkCloneableMessage.h"
#include "third_party/WebKit/common/message_port/message_port_channel.h"
#include "third_party/WebKit/common/message_port/transferable_message.h"

namespace blink {

// This struct represents messages as they are posted over a message port. This
// type can be serialized as a blink::mojom::TransferableMessage struct.
// This is the renderer-side equivalent of blink::TransferableMessage, where
// this struct uses blink types, while the other struct uses std:: types.
struct CORE_EXPORT BlinkTransferableMessage : BlinkCloneableMessage {
  BlinkTransferableMessage();
  ~BlinkTransferableMessage();

  BlinkTransferableMessage(BlinkTransferableMessage&&);
  BlinkTransferableMessage& operator=(BlinkTransferableMessage&&);

  Vector<MessagePortChannel> ports;

 private:
  DISALLOW_COPY_AND_ASSIGN(BlinkTransferableMessage);
};

CORE_EXPORT BlinkTransferableMessage
    ToBlinkTransferableMessage(TransferableMessage);
// Returned message will still be backed by the SerializedScriptValue in the
// input message, so is only valid as long as that SerializedScriptValue is
// alive. Call EnsureDataIsOwned on the returned message if you need it to live
// longer.
CORE_EXPORT TransferableMessage ToTransferableMessage(BlinkTransferableMessage);

}  // namespace blink

#endif  // BlinkTransferableMessage_h

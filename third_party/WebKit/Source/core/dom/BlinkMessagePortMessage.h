// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BlinkMessagePortMessage_h
#define BlinkMessagePortMessage_h

#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "third_party/WebKit/common/message_port/message_port_channel.h"

namespace blink {

namespace mojom {
namespace blink {
class MessagePortMessage;
}
}  // namespace mojom

struct BlinkMessagePortMessage {
  using MojoStruct = mojom::blink::MessagePortMessage;
  RefPtr<blink::SerializedScriptValue> message;
  Vector<MessagePortChannel> ports;
};

}  // namespace blink

#endif  // BlinkMessagePortMessage_h

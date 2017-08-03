// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MessagePortMessage_h
#define MessagePortMessage_h

#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "third_party/WebKit/common/message_port/message_port.h"

namespace blink {

struct BlinkMessagePortMessage {
  RefPtr<blink::SerializedScriptValue> message;
  std::vector<blink_common::MessagePort> ports;
};

}  // namespace blink

#endif  // MessagePortMessage_h

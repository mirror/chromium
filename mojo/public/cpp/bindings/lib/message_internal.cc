// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/lib/message_internal.h"

#include "mojo/public/cpp/bindings/lib/control_message_handler.h"

namespace mojo {
namespace internal {

bool ValidateUnserializedOrControlMessage(Message* message) {
  return !message->is_serialized() ||
         ControlMessageHandler::IsControlMessage(message);
}

}  // namespace internal
}  // namespace mojo

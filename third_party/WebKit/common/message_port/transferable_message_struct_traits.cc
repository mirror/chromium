// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/common/message_port/transferable_message_struct_traits.h"

#include "base/containers/span.h"
#include "third_party/WebKit/common/message_port/cloneable_message_struct_traits.h"

namespace mojo {

base::span<const uint8_t>
StructTraits<blink::mojom::TransferableMessage::DataView,
             blink::TransferableMessage>::
    sender_call_stack_id(blink::TransferableMessage& input) {
  return input.sender_call_stack_id;
}

bool StructTraits<blink::mojom::TransferableMessage::DataView,
                  blink::TransferableMessage>::
    Read(blink::mojom::TransferableMessage::DataView data,
         blink::TransferableMessage* out) {
  std::vector<mojo::ScopedMessagePipeHandle> ports;
  if (!data.ReadMessage(static_cast<blink::CloneableMessage*>(out)) ||
      !data.ReadPorts(&ports) ||
      !data.ReadSenderCallStackId(&out->sender_call_stack_id)) {
    return false;
  }

  out->ports = blink::MessagePortChannel::CreateFromHandles(std::move(ports));
  return true;
}

}  // namespace mojo

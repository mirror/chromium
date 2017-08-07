// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/common/message_port/message_port_message_struct_traits.h"

namespace mojo {

bool StructTraits<blink_common::mojom::MessagePortMessage::DataView,
                  blink_common::MessagePortMessage>::
    Read(blink_common::mojom::MessagePortMessage::DataView data,
         blink_common::MessagePortMessage* out) {
  std::vector<mojo::ScopedMessagePipeHandle> ports;
  if (!data.ReadEncodedMessage(&out->owned_encoded_message) ||
      !data.ReadPorts(&ports) || !data.ReadBlobs(&out->blobs))
    return false;

  out->encoded_message = mojo::ConstCArray<uint8_t>(
      out->owned_encoded_message.size(), out->owned_encoded_message.data());
  out->ports = blink_common::MessagePort::BindHandles(std::move(ports));
  return true;
}

}  // namespace mojo

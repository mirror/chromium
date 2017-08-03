// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/MessagePortMessageStructTraits.h"

namespace mojo {

bool StructTraits<blink_common::mojom::blink::MessagePortMessage::DataView,
                  blink::BlinkMessagePortMessage>::
    Read(blink_common::mojom::blink::MessagePortMessage::DataView data,
         blink::BlinkMessagePortMessage* out) {
  std::vector<mojo::ScopedMessagePipeHandle> ports;
  Vector<storage::mojom::blink::SerializedBlobPtr> blobs;
  if (!data.ReadPorts(&ports) || !data.ReadBlobs(&blobs))
    return false;

  mojo::ArrayDataView<uint8_t> message_data;
  data.GetEncodedMessageDataView(&message_data);
  out->message = blink::SerializedScriptValue::Create(
      reinterpret_cast<const char*>(message_data.data()), message_data.size());
  out->ports = blink_common::MessagePort::BindHandles(std::move(ports));
  for (auto& blob : blobs) {
    out->message->BlobDataHandles().Set(
        blob->uuid,
        blink::BlobDataHandle::Create(blob->uuid, blob->content_type,
                                      blob->size, blob->blob.PassInterface()));
  }
  return true;
}

}  // namespace mojo

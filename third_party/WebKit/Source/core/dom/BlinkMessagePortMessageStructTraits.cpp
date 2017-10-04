// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/BlinkMessagePortMessageStructTraits.h"

#include "platform/blob/BlobData.h"
#include "platform/runtime_enabled_features.h"

namespace mojo {

Vector<blink::mojom::blink::SerializedBlobPtr> StructTraits<
    blink::mojom::blink::MessagePortMessage::DataView,
    blink::BlinkMessagePortMessage>::blobs(blink::BlinkMessagePortMessage&
                                               input) {
  Vector<blink::mojom::blink::SerializedBlobPtr> result;
  if (blink::RuntimeEnabledFeatures::MojoBlobsEnabled()) {
    result.ReserveInitialCapacity(input.message->BlobDataHandles().size());
    for (const auto& blob : input.message->BlobDataHandles()) {
      result.push_back(blink::mojom::blink::SerializedBlob::New(
          blob.value->Uuid(), blob.value->GetType(), blob.value->size(),
          blob.value->CloneBlobPtr()));
    }
  }
  return result;
}

bool StructTraits<blink::mojom::blink::MessagePortMessage::DataView,
                  blink::BlinkMessagePortMessage>::
    Read(blink::mojom::blink::MessagePortMessage::DataView data,
         blink::BlinkMessagePortMessage* out) {
  std::vector<mojo::ScopedMessagePipeHandle> ports;
  Vector<blink::mojom::blink::SerializedBlobPtr> blobs;
  if (!data.ReadPorts(&ports) || !data.ReadBlobs(&blobs))
    return false;
  auto channels =
      blink::MessagePortChannel::CreateFromHandles(std::move(ports));
  out->ports.ReserveInitialCapacity(channels.size());
  out->ports.AppendRange(std::make_move_iterator(channels.begin()),
                         std::make_move_iterator(channels.end()));

  mojo::ArrayDataView<uint8_t> message_data;
  data.GetEncodedMessageDataView(&message_data);
  out->message = blink::SerializedScriptValue::Create(
      reinterpret_cast<const char*>(message_data.data()), message_data.size());
  for (auto& blob : blobs) {
    out->message->BlobDataHandles().Set(
        blob->uuid,
        blink::BlobDataHandle::Create(blob->uuid, blob->content_type,
                                      blob->size, blob->blob.PassInterface()));
  }
  return true;
}

}  // namespace mojo

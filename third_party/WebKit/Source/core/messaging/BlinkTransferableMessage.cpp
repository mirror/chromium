// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/messaging/BlinkTransferableMessage.h"

#include "platform/blob/BlobData.h"
#include "public/platform/WebString.h"
#include "third_party/WebKit/common/message_port/message_port.mojom.h"

namespace blink {

BlinkTransferableMessage::BlinkTransferableMessage() = default;
BlinkTransferableMessage::~BlinkTransferableMessage() = default;

BlinkTransferableMessage::BlinkTransferableMessage(BlinkTransferableMessage&&) =
    default;
BlinkTransferableMessage& BlinkTransferableMessage::operator=(
    BlinkTransferableMessage&&) = default;

// static
BlinkTransferableMessage BlinkTransferableMessage::FromCommon(
    TransferableMessage message) {
  BlinkTransferableMessage result;
  result.message = SerializedScriptValue::Create(
      reinterpret_cast<const char*>(message.encoded_message.data()),
      message.encoded_message.length());
  for (auto& blob : message.blobs) {
    result.message->BlobDataHandles().Set(
        WebString::FromUTF8(blob->uuid),
        BlobDataHandle::Create(
            WebString::FromUTF8(blob->uuid),
            WebString::FromUTF8(blob->content_type), blob->size,
            mojom::blink::BlobPtrInfo(blob->blob.PassHandle(),
                                      mojom::Blob::Version_)));
  }
  result.sender_stack_trace_id = v8_inspector::V8StackTraceId(
      message.stack_trace_id,
      std::make_pair(message.stack_trace_debugger_id_first,
                     message.stack_trace_debugger_id_second));
  result.ports.AppendRange(message.ports.begin(), message.ports.end());
  return result;
}

TransferableMessage BlinkTransferableMessage::AsCommon() {
  TransferableMessage result;
  result.encoded_message = message->GetWireData();
  result.blobs.reserve(message->BlobDataHandles().size());
  for (const auto& blob : message->BlobDataHandles()) {
    result.blobs.push_back(mojom::SerializedBlob::New(
        WebString(blob.value->Uuid()).Utf8(),
        WebString(blob.value->GetType()).Utf8(), blob.value->size(),
        mojom::BlobPtrInfo(
            blob.value->CloneBlobPtr().PassInterface().PassHandle(),
            mojom::Blob::Version_)));
  }
  result.stack_trace_id = sender_stack_trace_id.id;
  result.stack_trace_debugger_id_first =
      sender_stack_trace_id.debugger_id.first;
  result.stack_trace_debugger_id_second =
      sender_stack_trace_id.debugger_id.second;
  result.ports.assign(ports.begin(), ports.end());
  return result;
}

}  // namespace blink

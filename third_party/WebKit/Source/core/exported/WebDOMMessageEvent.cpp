/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "public/web/WebDOMMessageEvent.h"

#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "core/dom/Document.h"
#include "core/events/MessageEvent.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/messaging/MessagePort.h"
#include "public/platform/WebString.h"
#include "public/web/WebDocument.h"
#include "public/web/WebFrame.h"
#include "public/web/WebSerializedScriptValue.h"
#include "third_party/WebKit/common/message_port/message_port.mojom.h"

namespace blink {

namespace {

scoped_refptr<SerializedScriptValue>
SerializedScriptValueFromTransferableMessage(TransferableMessage* message) {
  auto result = SerializedScriptValue::Create(
      reinterpret_cast<const char*>(message->encoded_message.data()),
      message->encoded_message.length());
  for (auto& blob : message->blobs) {
    result->BlobDataHandles().Set(
        WebString::FromUTF8(blob->uuid),
        BlobDataHandle::Create(
            WebString::FromUTF8(blob->uuid),
            WebString::FromUTF8(blob->content_type), blob->size,
            mojom::blink::BlobPtrInfo(blob->blob.PassHandle(),
                                      mojom::Blob::Version_)));
  }
  return result;
}

}  // namespace

WebDOMMessageEvent::WebDOMMessageEvent(
    const WebSerializedScriptValue& message_data,
    const WebString& origin,
    const WebFrame* source_frame,
    const WebDocument& target_document,
    WebVector<MessagePortChannel> channels)
    : WebDOMMessageEvent(MessageEvent::Create()) {
  DOMWindow* window = nullptr;
  if (source_frame)
    window = WebFrame::ToCoreFrame(*source_frame)->DomWindow();
  MessagePortArray* ports = nullptr;
  if (!target_document.IsNull()) {
    Document* core_document = target_document;
    ports = MessagePort::EntanglePorts(*core_document, std::move(channels));
  }
  // Use an empty array for |ports| when it is null because this function
  // is used to implement postMessage().
  if (!ports)
    ports = new MessagePortArray;
  // TODO(esprehn): Chromium always passes empty string for lastEventId, is that
  // right?
  Unwrap<MessageEvent>()->initMessageEvent("message", false, false,
                                           message_data, origin,
                                           "" /*lastEventId*/, window, ports);
}

WebDOMMessageEvent::WebDOMMessageEvent(TransferableMessage message,
                                       const WebString& origin,
                                       const WebFrame* source_frame,
                                       const WebDocument& target_document)
    : WebDOMMessageEvent(SerializedScriptValueFromTransferableMessage(&message),
                         origin,
                         source_frame,
                         target_document,
                         message.ports) {}

WebString WebDOMMessageEvent::Origin() const {
  return WebString(ConstUnwrap<MessageEvent>()->origin());
}

TransferableMessage WebDOMMessageEvent::ReleaseMessage() {
  TransferableMessage result;
  SerializedScriptValue* value =
      Unwrap<MessageEvent>()->DataAsSerializedScriptValue();
  result.encoded_message = value->GetWireData();
  auto ports = Unwrap<MessageEvent>()->ReleaseChannels();
  result.ports.assign(ports.begin(), ports.end());
  result.blobs.reserve(value->BlobDataHandles().size());
  for (const auto& blob : value->BlobDataHandles()) {
    result.blobs.push_back(mojom::SerializedBlob::New(
        WebString(blob.value->Uuid()).Utf8(),
        WebString(blob.value->GetType()).Utf8(), blob.value->size(),
        mojom::BlobPtrInfo(
            blob.value->CloneBlobPtr().PassInterface().PassHandle(),
            mojom::Blob::Version_)));
  }
  return result;
}

}  // namespace blink

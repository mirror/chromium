// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/gcm_receiver_typemap_traits.h"

#include "base/logging.h"
#include "mojo/common/common_custom_types_struct_traits.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace mojo {

// static
gcm::mojom::GCMResult
EnumTraits<gcm::mojom::GCMResult, gcm::GCMClient::Result>::ToMojom(
    gcm::GCMClient::Result result) {
  NOTREACHED();
  return gcm::mojom::GCMResult::SUCCESS;
}

// static
bool EnumTraits<gcm::mojom::GCMResult, gcm::GCMClient::Result>::FromMojom(
    gcm::mojom::GCMResult input,
    gcm::GCMClient::Result* out) {
  return false;
}

std::map<std::string, std::string>
StructTraits<gcm::mojom::GCMIncomingMessageDataView,
             gcm::IncomingMessage>::data(const gcm::IncomingMessage& input) {
  return input.data;
}

std::string StructTraits<
    gcm::mojom::GCMIncomingMessageDataView,
    gcm::IncomingMessage>::collapse_key(const gcm::IncomingMessage& input) {
  return input.collapse_key;
}

std::string StructTraits<
    gcm::mojom::GCMIncomingMessageDataView,
    gcm::IncomingMessage>::sender_id(const gcm::IncomingMessage& input) {
  return input.sender_id;
}

std::string StructTraits<
    gcm::mojom::GCMIncomingMessageDataView,
    gcm::IncomingMessage>::raw_data(const gcm::IncomingMessage& input) {
  return input.raw_data;
}

bool StructTraits<gcm::mojom::GCMIncomingMessageDataView,
                  gcm::IncomingMessage>::decrypted(const gcm::IncomingMessage&
                                                       input) {
  return input.decrypted;
}

bool StructTraits<
    gcm::mojom::GCMIncomingMessageDataView,
    gcm::IncomingMessage>::Read(gcm::mojom::GCMIncomingMessageDataView data,
                                gcm::IncomingMessage* output) {
  output->decrypted = data.decrypted();
  if (!data.ReadData<gcm::MessageData>(&output->data) ||
      !data.ReadCollapseKey<std::string>(&output->collapse_key) ||
      !data.ReadSenderId<std::string>(&output->sender_id) ||
      !data.ReadRawData<std::string>(&output->raw_data)) {
    return false;
  }
  return true;
}

std::string StructTraits<gcm::mojom::GCMSendErrorDetailsDataView,
                         gcm::GCMClient::SendErrorDetails>::
    message_id(const gcm::GCMClient::SendErrorDetails& input) {
  return input.message_id;
}

gcm::MessageData StructTraits<gcm::mojom::GCMSendErrorDetailsDataView,
                              gcm::GCMClient::SendErrorDetails>::
    additional_data(const gcm::GCMClient::SendErrorDetails& input) {
  return input.additional_data;
}

gcm::mojom::GCMResult StructTraits<gcm::mojom::GCMSendErrorDetailsDataView,
                                   gcm::GCMClient::SendErrorDetails>::
    result(const gcm::GCMClient::SendErrorDetails& input) {
  return EnumTraits<gcm::mojom::GCMResult, gcm::GCMClient::Result>::ToMojom(
      input.result);
}

bool StructTraits<gcm::mojom::GCMSendErrorDetailsDataView,
                  gcm::GCMClient::SendErrorDetails>::
    Read(gcm::mojom::GCMSendErrorDetailsDataView data,
         gcm::GCMClient::SendErrorDetails* output) {
  if (!data.ReadMessageId<std::string>(&output->message_id) ||
      !data.ReadAdditionalData<gcm::MessageData>(&output->additional_data) ||
      !data.ReadResult<gcm::GCMClient::Result>(&output->result)) {
    return false;
  }
  return true;
}

}  // namespace mojo

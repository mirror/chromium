// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GCM_DRIVER_MOJO_GCM_RECEIVER_TYPEMAP_TRAITS_H_
#define COMPONENTS_GCM_DRIVER_MOJO_GCM_RECEIVER_TYPEMAP_TRAITS_H_

#include "base/numerics/safe_conversions.h"
#include "components/gcm_driver/common/gcm_messages.h"
#include "components/gcm_driver/gcm_client.h"
#include "components/gcm_driver/public/interfaces/gcm_receiver.mojom.h"

namespace mojo {

template <>
struct EnumTraits<gcm::mojom::GCMResult, gcm::GCMClient::Result> {
  static gcm::mojom::GCMResult ToMojom(gcm::GCMClient::Result result);

  static bool FromMojom(gcm::mojom::GCMResult input,
                        gcm::GCMClient::Result* out);
};

template <>
struct StructTraits<gcm::mojom::GCMIncomingMessageDataView,
                    gcm::IncomingMessage> {
  static gcm::MessageData data(const gcm::IncomingMessage& input);
  static std::string collapse_key(const gcm::IncomingMessage& input);
  static std::string sender_id(const gcm::IncomingMessage& input);
  static std::string raw_data(const gcm::IncomingMessage& input);
  static bool decrypted(const gcm::IncomingMessage& input);
  static bool Read(gcm::mojom::GCMIncomingMessageDataView data,
                   gcm::IncomingMessage* output);
};

template <>
struct StructTraits<gcm::mojom::GCMSendErrorDetailsDataView,
                    gcm::GCMClient::SendErrorDetails> {
  static std::string message_id(const gcm::GCMClient::SendErrorDetails& input);
  static gcm::MessageData additional_data(
      const gcm::GCMClient::SendErrorDetails& input);
  static gcm::mojom::GCMResult result(
      const gcm::GCMClient::SendErrorDetails& input);
  static bool Read(gcm::mojom::GCMSendErrorDetailsDataView data,
                   gcm::GCMClient::SendErrorDetails* output);
};

}  // namespace mojo

#endif  // COMPONENTS_GCM_DRIVER_MOJO_GCM_RECEIVER_TYPEMAP_TRAITS_H_

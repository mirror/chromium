// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_APDU_APDU_RESPONSE_H_
#define COMPONENTS_APDU_APDU_RESPONSE_H_

#include <memory>
#include <vector>

#include "base/gtest_prod_util.h"
#include "components/apdu/apdu_export.h"

namespace apdu {

// APDU responses are defined as part of ISO 7816-4. Serialized responses
// consist of a data field of varying length, up to a maximum 65536, and a
// two byte status field.
class APDU_EXPORT APDUResponse {
 public:
  // Status bytes are specified in ISO 7816-4
  enum class Status : uint16_t {
    SW_NO_ERROR = 0x9000,
    SW_CONDITIONS_NOT_SATISFIED = 0x6985,
    SW_WRONG_DATA = 0x6A80,
    SW_WRONG_LENGTH = 0x6700,
  };

  // Create a APDU response from the serialized message
  static std::unique_ptr<APDUResponse> CreateFromMessage(
      const std::vector<uint8_t>& data);

  APDUResponse(std::vector<uint8_t> message, Status response_status);
  ~APDUResponse();

  std::vector<uint8_t> GetEncodedResponse() const;

  const std::vector<uint8_t> data() const { return data_; }
  Status status() const { return response_status_; }

 private:
  FRIEND_TEST_ALL_PREFIXES(ApduTest, TestDeserializeResponse);

  Status response_status_;
  std::vector<uint8_t> data_;
};

}  // namespace apdu

#endif  // COMPONENTS_APDU_APDU_RESPONSE_H_

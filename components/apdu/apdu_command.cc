// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/apdu/apdu_command.h"

#include <utility>

#include "base/memory/ptr_util.h"

namespace apdu {

namespace {
// Non-ISO 7816 ins command that adds extra 2 byes to APDU encoded command.
static constexpr uint8_t kU2fVersionInsCommand = 0x03;
}  // namespace

std::unique_ptr<ApduCommand> ApduCommand::CreateFromMessageForTesting(
    const std::vector<uint8_t>& message) {
  uint16_t data_length = 0;
  size_t index = 0;
  size_t response_length = 0;
  std::vector<uint8_t> data;
  std::vector<uint8_t> suffix;

  if (message.size() < kApduMinHeader || message.size() > kApduMaxLength)
    return nullptr;
  uint8_t cla = message[index++];
  uint8_t ins = message[index++];
  uint8_t p1 = message[index++];
  uint8_t p2 = message[index++];

  switch (message.size()) {
    // No data present; no expected response.
    case kApduMinHeader:
      break;
    // Invalid encoding sizes.
    case kApduMinHeader + 1:
    case kApduMinHeader + 2:
      return nullptr;
    // No data present; response expected.
    case kApduMinHeader + 3:
      // Fifth byte must be 0
      if (message[index++] != 0)
        return nullptr;
      response_length = message[index++] << 8;
      response_length |= message[index++];
      // Special case where response length of 0x0000 corresponds to 65536.
      // Defined in ISO7816-4.
      if (response_length == 0)
        response_length = kApduMaxResponseLength;
      break;
    default:
      // Fifth byte must be 0.
      if (message[index++] != 0)
        return nullptr;
      data_length = message[index++] << 8;
      data_length |= message[index++];

      if (message.size() == data_length + index) {
        // No response expected.
        data.insert(data.end(), message.begin() + index, message.end());
      } else if (message.size() == data_length + index + 2) {
        // Maximum response size is stored in final 2 bytes.
        data.insert(data.end(), message.begin() + index, message.end() - 2);
        index += data_length;
        response_length = message[index++] << 8;
        response_length |= message[index++];
        // Special case where response length of 0x0000 corresponds to 65536.
        // Defined in ISO7816-4.
        if (response_length == 0)
          response_length = kApduMaxResponseLength;
        // Non-ISO7816-4 special legacy case where 2 suffix bytes are passed
        // along with a version message.
        if (data_length == 0 && ins == kU2fVersionInsCommand)
          suffix = {0x0, 0x0};
      } else {
        return nullptr;
      }
      break;
  }

  return std::make_unique<ApduCommand>(cla, ins, p1, p2, response_length,
                                       std::move(data), std::move(suffix));
}

ApduCommand::ApduCommand()
    : cla_(0), ins_(0), p1_(0), p2_(0), response_length_(0) {}

ApduCommand::ApduCommand(uint8_t cla,
                         uint8_t ins,
                         uint8_t p1,
                         uint8_t p2,
                         size_t response_length,
                         std::vector<uint8_t> data,
                         std::vector<uint8_t> suffix)
    : cla_(cla),
      ins_(ins),
      p1_(p1),
      p2_(p2),
      response_length_(response_length),
      data_(std::move(data)),
      suffix_(std::move(suffix)) {}

ApduCommand::~ApduCommand() = default;

std::vector<uint8_t> ApduCommand::GetEncodedCommand() const {
  std::vector<uint8_t> encoded = {cla_, ins_, p1_, p2_};

  // If data exists, request size (Lc) is encoded in 3 bytes, with the first
  // byte always being null, and the other two bytes being a big-endian
  // representation of the request size. If data length is 0, response size (Le)
  // will be prepended with a null byte.
  if (data_.size() > 0) {
    size_t data_length = data_.size();

    encoded.push_back(0x0);
    if (data_length > kApduMaxDataLength)
      data_length = kApduMaxDataLength;
    encoded.push_back((data_length >> 8) & 0xff);
    encoded.push_back(data_length & 0xff);
    encoded.insert(encoded.end(), data_.begin(), data_.begin() + data_length);
  } else if (response_length_ > 0) {
    encoded.push_back(0x0);
  }

  if (response_length_ > 0) {
    encoded.push_back((response_length_ >> 8) & 0xff);
    encoded.push_back(response_length_ & 0xff);
  }
  // Add suffix, if required, for legacy compatibility
  encoded.insert(encoded.end(), suffix_.begin(), suffix_.end());
  return encoded;
}

}  // namespace apdu

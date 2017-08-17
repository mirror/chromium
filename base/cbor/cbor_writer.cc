// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/cbor/cbor_writer.h"

#include <stdint.h>

#include "base/logging.h"
#include "base/numerics/safe_conversions.h"

namespace base {

CBORWriter::CBORWriter() {
  expected_items_count_ = 0;
}

void CBORWriter::WriteUint(uint64_t value) {
  InitialByteSetType(CborMajorType::kUnsigned);
  SetUint(value);

  if (expected_items_count_.ValueOrDie() > 0)
    expected_items_count_--;
}

void CBORWriter::WriteBytes(const std::vector<uint8_t>& bytes) {
  InitialByteSetType(CborMajorType::kByteString);

  // Encode the number of bytes.
  SetUint(strict_cast<uint64_t>(bytes.size()));

  // Add the bytes.
  encoded_cbor_.insert(encoded_cbor_.end(), bytes.begin(), bytes.end());

  if (expected_items_count_.ValueOrDie() > 0)
    expected_items_count_--;
}

void CBORWriter::WriteString(const std::string& string) {
  InitialByteSetType(CborMajorType::kString);

  SetUint(strict_cast<uint64_t>(string.size()));

  // Add the bytes.
  encoded_cbor_.insert(encoded_cbor_.end(), string.begin(), string.end());

  if (expected_items_count_.ValueOrDie() > 0)
    expected_items_count_--;
}

void CBORWriter::WriteMap(size_t len) {
  // To avoid overflow in multiplicatin.
  DCHECK_LT(len, std::numeric_limits<size_t>::max() / 2);

  InitialByteSetType(CborMajorType::kMap);
  SetUint(len);
  // Maintain count of the key,value pairs expected to follow.
  expected_items_count_ += len * 2;
}

void CBORWriter::InitialByteSetType(CborMajorType type) {
  encoded_cbor_.push_back(base::checked_cast<uint8_t>(type) << 5);
}

void CBORWriter::InitialByteSetAdditionalInformation(
    uint8_t additional_information) {
  encoded_cbor_[encoded_cbor_.size() - 1] =
      encoded_cbor_[encoded_cbor_.size() - 1] |
      (additional_information & kAdditionalInformationDataMask);
}

void CBORWriter::SetUint(uint64_t value) {
  size_t count = GetNumUintBytes(value);

  // Values under 24 are encoded directly in the initial byte.
  // Otherwise, the last 5 bites of the initial byte contains the length
  // of the additional information, and the additional information is
  // encoded in following bytes.
  switch (count) {
    case 0:
      InitialByteSetAdditionalInformation(base::checked_cast<uint8_t>(value));
      return;
    case 1:
      InitialByteSetAdditionalInformation(kAdditionalInformation1Byte);
      encoded_cbor_.push_back(base::checked_cast<uint8_t>(value));
      return;
    case 2:
      InitialByteSetAdditionalInformation(kAdditionalInformation2Bytes);
      encoded_cbor_.push_back(
          base::checked_cast<uint8_t>((0xFF & (value >> 8))));
      encoded_cbor_.push_back(base::checked_cast<uint8_t>((0xFF & (value))));
      return;
    case 4:
      InitialByteSetAdditionalInformation(kAdditionalInformation4Bytes);
      encoded_cbor_.push_back(
          base::checked_cast<uint8_t>((0xFF & (value >> 24))));
      encoded_cbor_.push_back(
          base::checked_cast<uint8_t>((0xFF & (value >> 16))));
      encoded_cbor_.push_back(
          base::checked_cast<uint8_t>((0xFF & (value >> 8))));
      encoded_cbor_.push_back(base::checked_cast<uint8_t>((0xFF & (value))));
      return;
    case 8:
      InitialByteSetAdditionalInformation(kAdditionalInformation8Bytes);
      encoded_cbor_.push_back(
          base::checked_cast<uint8_t>((0xFF & (value >> 56))));
      encoded_cbor_.push_back(
          base::checked_cast<uint8_t>((0xFF & (value >> 48))));
      encoded_cbor_.push_back(
          base::checked_cast<uint8_t>((0xFF & (value >> 40))));
      encoded_cbor_.push_back(
          base::checked_cast<uint8_t>((0xFF & (value >> 32))));
      encoded_cbor_.push_back(
          base::checked_cast<uint8_t>((0xFF & (value >> 24))));
      encoded_cbor_.push_back(
          base::checked_cast<uint8_t>((0xFF & (value >> 16))));
      encoded_cbor_.push_back(
          base::checked_cast<uint8_t>((0xFF & (value >> 8))));
      encoded_cbor_.push_back(base::checked_cast<uint8_t>((0xFF & (value))));
      return;
  }
  NOTREACHED();
}

size_t CBORWriter::GetNumUintBytes(uint64_t value) {
  if (value < 24) {
    return 0;
  } else if (value <= 0xFF) {
    return 1;
  } else if (value <= 0xFFFF) {
    return 2;
  } else if (value <= 0xFFFFFFFF) {
    return 4;
  }
  return 8;
}

std::vector<uint8_t> CBORWriter::Serialize() {
  // Verify that there are no more items expected from a map declaration.
  DCHECK(expected_items_count_.ValueOrDie() == 0);
  return encoded_cbor_;
}

}  // namespace base

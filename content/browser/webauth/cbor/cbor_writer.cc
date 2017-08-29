// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/cbor/cbor_writer.h"

#include <stdint.h>

#include "base/numerics/safe_conversions.h"

namespace content {

CBORWriter::CBORWriter() {
  expected_items_count_ = 0;
}

void CBORWriter::WriteUint(uint64_t value) {
  StartItem(CborMajorType::kUnsigned, value);

  if (expected_items_count_.ValueOrDie() > 0)
    expected_items_count_--;
}

void CBORWriter::WriteBytes(const std::vector<uint8_t>& bytes) {
  StartItem(CborMajorType::kByteString,
            base::strict_cast<uint64_t>(bytes.size()));

  // Add the bytes.
  encoded_cbor_.insert(encoded_cbor_.end(), bytes.begin(), bytes.end());

  if (expected_items_count_.ValueOrDie() > 0)
    expected_items_count_--;
}

void CBORWriter::WriteString(const base::StringPiece& string) {
  StartItem(CborMajorType::kString, base::strict_cast<uint64_t>(string.size()));

  // Add the bytes.
  encoded_cbor_.insert(encoded_cbor_.end(), string.begin(), string.end());

  if (expected_items_count_.ValueOrDie() > 0)
    expected_items_count_--;
}

void CBORWriter::DeclareArray(size_t len) {
  StartItem(CborMajorType::kArray, len);
  if (expected_items_count_.ValueOrDie() > 0) {
    // Decrement for nested arrays or maps.
    expected_items_count_--;
  }
  // Maintain count of the elements expected to follow.
  expected_items_count_ += len;
}

void CBORWriter::DeclareMap(size_t len) {
  // To avoid overflow in multiplicatin.
  DCHECK_LT(len, std::numeric_limits<size_t>::max() / 2);

  StartItem(CborMajorType::kMap, len);
  if (expected_items_count_.ValueOrDie() > 0) {
    // Decrement for nested arrays or maps.
    expected_items_count_--;
  }
  // Maintain count of the key,value pairs expected to follow.
  expected_items_count_ += len * 2;
}

void CBORWriter::StartItem(CborMajorType type, uint64_t size) {
  encoded_cbor_.push_back(base::checked_cast<uint8_t>(type) << 5);
  SetUint(size);
}

void CBORWriter::SetAdditionalInformation(uint8_t additional_information) {
  encoded_cbor_[encoded_cbor_.size() - 1] |=
      (additional_information & kAdditionalInformationDataMask);
}

void CBORWriter::SetUint(uint64_t value) {
  size_t count = GetNumUintBytes(value);
  int shift = -1;
  // Values under 24 are encoded directly in the initial byte.
  // Otherwise, the last 5 bits of the initial byte contains the length
  // of unsigned integer, which is encoded in following bytes.
  switch (count) {
    case 0:
      SetAdditionalInformation(base::checked_cast<uint8_t>(value));
      break;
    case 1:
      SetAdditionalInformation(kAdditionalInformation1Byte);
      shift = 0;
      break;
    case 2:
      SetAdditionalInformation(kAdditionalInformation2Bytes);
      shift = 1;
      break;
      break;
    case 4:
      SetAdditionalInformation(kAdditionalInformation4Bytes);
      shift = 3;
      break;
      return;
    case 8:
      SetAdditionalInformation(kAdditionalInformation8Bytes);
      shift = 7;
      break;
  }
  for (; shift >= 0; shift--) {
    encoded_cbor_.push_back(
        base::checked_cast<uint8_t>(0xFF & (value >> (shift * 8))));
  }
  return;
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

}  // namespace content

// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/cbor/cbor_writer.h"

#include <stdint.h>

#include "base/logging.h"

namespace base {

CBORWriter::CBORWriter() {
  expected_items_count_ = 0;
}

void CBORWriter::WriteUint(uint32_t value) {
  SetType(MAJOR_TYPE_0);
  SetUint(value);

  if (expected_items_count_ > 0)
    expected_items_count_--;
}

void CBORWriter::WriteBytes(const std::vector<uint8_t> bytes) {
  SetType(MAJOR_TYPE_2);

  // Encode the number of bytes.
  SetUint((uint32_t)bytes.size());

  // Add the bytes.
  encoded_cbor_.insert(encoded_cbor_.end(), bytes.begin(), bytes.end());

  if (expected_items_count_ > 0)
    expected_items_count_--;
}

void CBORWriter::WriteString(const std::string& string) {
  SetType(MAJOR_TYPE_3);

  SetUint((uint32_t)string.size());

  // Add the bytes.
  encoded_cbor_.insert(encoded_cbor_.end(), string.begin(), string.end());

  if (expected_items_count_ > 0)
    expected_items_count_--;
}

void CBORWriter::WriteMap(size_t len) {
  DCHECK(len > 0);
  SetType(MAJOR_TYPE_5);
  SetUint(len);
  // Maintain count of the key,value pairs expected to follow.
  expected_items_count_ = len * 2;
}

void CBORWriter::SetType(CBOR_MAJOR_TYPE type) {
  DCHECK(type <= 5);
  encoded_cbor_.push_back((uint8_t)type << 5);
}

void CBORWriter::SetDetails(uint8_t byte_details) {
  encoded_cbor_[encoded_cbor_.size() - 1] =
      encoded_cbor_[encoded_cbor_.size() - 1] |
      (byte_details & DETAILS_DATA_MASK);
}

void CBORWriter::SetUint(uint32_t value) {
  size_t count = GetNumUintBytes(value);

  switch (count) {
    case 0:
      // Values under 24 are encoded directly in the 5 detail bits.
      SetDetails((uint8_t)value);
      break;
    case 1:
      SetDetails(DETAILS_1BYTE);
      encoded_cbor_.push_back((uint8_t)value);
      break;
    case 2:
      SetDetails(DETAILS_2BYTE);
      encoded_cbor_.push_back((uint8_t)(0xFF & (value >> 8)));
      encoded_cbor_.push_back((uint8_t)(0xFF & (value)));
      break;
    case 4:
      SetDetails(DETAILS_4BYTE);
      encoded_cbor_.push_back((uint8_t)(0xFF & (value >> 24)));
      encoded_cbor_.push_back((uint8_t)(0xFF & (value >> 16)));
      encoded_cbor_.push_back((uint8_t)(0xFF & (value >> 8)));
      encoded_cbor_.push_back((uint8_t)(0xFF & (value)));
      break;
  }
}

size_t CBORWriter::GetNumUintBytes(uint32_t value) {
  if (value < 24) {
    return 0;
  } else if (value <= 0xFF) {
    return 1;
  } else if (value <= 0xFFFF) {
    return 2;
  }
  return 4;
}

std::vector<uint8_t> CBORWriter::Serialize() {
  // Verify that there are no more items expected from a map declaration.
  DCHECK(expected_items_count_ == 0);
  return encoded_cbor_;
}

}  // namespace base

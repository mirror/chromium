// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_CBOR_CBOR_WRITER_H_
#define BASE_CBOR_CBOR_WRITER_H_

#include <string>
#include <vector>

#include "base/base_export.h"

namespace base {

#define MAJOR_TYPE_MASK 0xE0    // The first 3 bits encode the type.
#define DETAILS_DATA_MASK 0x1F  // The last 5 bits encode details.
#define DETAILS_1BYTE 24  // Indicates the integer is in the following byte.
#define DETAILS_2BYTE 25  // Indicates the integer is in the next 2 bytes.
#define DETAILS_4BYTE 26  // Indicates the integer is in the next 4 bytes.

/* A basic CBOR encoder as defined by https://tools.ietf.org/html/rfc7049.
   This implementation currently handles Major Types 1, 2, 3, and 5.*/
class BASE_EXPORT CBORWriter {
 public:
  CBORWriter();
  ~CBORWriter();

  // Encodes an unsigned integer (major type 0).
  void WriteUint(uint32_t value);

  // Encodes |len| number of |bytes| (major type 2).
  void WriteBytes(const std::vector<uint8_t> bytes);

  // Encodes a UTF-8|string| (major type 3).
  void WriteString(const std::string& string);

  // Declare a map (major type 5).
  // This expects to be followed by |len| number of key/value pairs.
  // This implementation does not validate key/value pairs (such as ensuring
  // unique keys) other than to ensure that at least len*2 items have been
  // added following the map declaration.
  void WriteMap(size_t len);

  // Validates and returns the encoded CBOR.
  std::vector<uint8_t> Serialize();

 private:
  enum CBOR_MAJOR_TYPE {
    MAJOR_TYPE_0,  // Unsigned integer.
    MAJOR_TYPE_1,  // Signed integer. Unsupported by this implementation.
    MAJOR_TYPE_2,  // Byte string.
    MAJOR_TYPE_3,  // String.
    MAJOR_TYPE_4,  // Array. Unsupported by this implementation.
    MAJOR_TYPE_5,  // Map.
  };

  // Encodes the type of the data being added.
  void SetType(CBOR_MAJOR_TYPE type);
  void SetDetails(uint8_t byte_details);  // Encodes the details for the data.
  // Encodes an unsigned integer value. This is called for both writing
  // unsigned integers and for encoding the lengths of other major types.
  void SetUint(uint32_t value);

  // Get the number of bytes needed to store the unsigned integer.
  size_t GetNumUintBytes(uint32_t value);

  // Holds the encoded CBOR data.
  std::vector<uint8_t> encoded_cbor_;

  // Maintains count of expected items following a map declaration.
  int expected_items_count_;
};

}  // namespace base

#endif  // BASE_CBOR_CBOR_WRITER_H_

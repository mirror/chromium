// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_CBOR_CBOR_WRITER_H_
#define BASE_CBOR_CBOR_WRITER_H_

#include <string>
#include <vector>

#include "base/base_export.h"
#include "base/numerics/safe_math.h"
/* A basic CBOR encoder as defined by https://tools.ietf.org/html/rfc7049.
 * Supported:
 *  * Major types:
 *     * 0: Unsigned integers, up to 64-bit.
 *     * 2 & 3: Byte and UTF-8 strings.
 *     * 5: Maps, with the number of elements known at the start
 *              of the container.
 * Unsupported:
 *  * Negative integers
 *  * Floating-point numbers
 *  * Arrays
 *  * Indefinite-length encodings.
 *  * Parsing
 */
enum class CborMajorType {
  kUnsigned = 0,    // Unsigned integer.
  kNegative = 1,    // Negative integer. Unsupported by this implementation.
  kByteString = 2,  // Byte string.
  kString = 3,      // String.
  kArray = 4,       // Array. Unsupported by this implementation.
  kMap = 5,         // Map.
};

namespace base {

// The last 5 bits encode additional information.
constexpr uint8_t kAdditionalInformationDataMask = 0x1F;
// Indicates the integer is in the following byte.
constexpr uint8_t kAdditionalInformation1Byte = 24;
// Indicates the integer is in the next 2 bytes.
constexpr uint8_t kAdditionalInformation2Bytes = 25;
// Indicates the integer is in the next 4 bytes.
constexpr uint8_t kAdditionalInformation4Bytes = 26;
// Indicates the integer is in the next 8 bytes.
constexpr uint8_t kAdditionalInformation8Bytes = 27;

class BASE_EXPORT CBORWriter {
 public:
  CBORWriter();
  ~CBORWriter();

  // Encodes an unsigned integer (major type 0).
  void WriteUint(uint64_t value);

  // Encodes |len| number of |bytes| (major type 2).
  void WriteBytes(const std::vector<uint8_t>& bytes);

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
  // Encodes the type and size of the data being added.
  void StartItem(CborMajorType type, uint64_t size);

  // Encodes the additional information for the data.
  void SetAdditionalInformation(uint8_t additional_information);

  // Encodes an unsigned integer value. This is used to both write
  // unsigned integers and to encode the lengths of other major types.
  void SetUint(uint64_t value);

  // Get the number of bytes needed to store the unsigned integer.
  size_t GetNumUintBytes(uint64_t value);

  // Holds the encoded CBOR data.
  std::vector<uint8_t> encoded_cbor_;

  // Maintains count of expected items following a map declaration.
  CheckedNumeric<int> expected_items_count_;
};

}  // namespace base

#endif  // BASE_CBOR_CBOR_WRITER_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_WRITER_H_
#define CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_WRITER_H_

#include <string>
#include <vector>

#include "base/numerics/safe_math.h"
#include "base/strings/string_piece.h"
#include "content/common/content_export.h"

// A basic Concise Binary Object Representation (CBOR) encoder as defined by
// https://tools.ietf.org/html/rfc7049.
// Supported:
//  * Major types:
//     * 0: Unsigned integers, up to 64-bit.
//     * 2: Byte strings.
//     * 3: UTF-8 strings.
//     * 4: Arrays, with the number of elements known at the start.
//     * 5: Maps, with the number of elements known at the start
//              of the container.
// Unsupported:
//  * Negative integers.
//  * Floating-point numbers.
//  * Arrays.
//  * Indefinite-length encodings.
//  * Parsing.
enum class CborMajorType {
  kUnsigned = 0,    // Unsigned integer.
  kNegative = 1,    // Negative integer. Unsupported by this implementation.
  kByteString = 2,  // Byte string.
  kString = 3,      // String.
  kArray = 4,       // Array.
  kMap = 5,         // Map.
};

namespace content {

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

class CONTENT_EXPORT CBORWriter {
 public:
  CBORWriter();
  ~CBORWriter();

  // Encodes an unsigned integer.
  void WriteUint(uint64_t value);

  // Encodes |bytes|.
  void WriteBytes(const std::vector<uint8_t>& bytes);

  // Encodes a UTF-8|string|.
  void WriteString(const base::StringPiece& string);

  // Declare an array.
  // This expects to be followed by |len| number of values.
  void DeclareArray(size_t len);

  // Declare a map.
  // This expects to be followed by |len| number of key/value pairs.
  // This implementation does not validate key/value pairs (such as ensuring
  // unique keys) other than to ensure that at least len*2 items have been
  // added following the map declaration.
  void DeclareMap(size_t len);

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
  base::CheckedNumeric<int> expected_items_count_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_WRITER_H_

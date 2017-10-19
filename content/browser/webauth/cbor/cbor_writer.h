// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_WRITER_H_
#define CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_WRITER_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/numerics/safe_math.h"
#include "base/strings/string_piece.h"
#include "content/browser/webauth/cbor/cbor_values.h"
#include "content/common/content_export.h"

#include "base/gtest_prod_util.h"

// A basic Concise Binary Object Representation (CBOR) encoder as defined by
// https://tools.ietf.org/html/rfc7049.
// This is a generic encoder that supplies well-formed
// CBOR values but does not guarantee their validity (see
// https://tools.ietf.org/html/rfc7049#section-3.2).
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
//  * Indefinite-length encodings.
//  * Parsing.
//
// Requirements for canonical CBOR to be used for CTAP are:
//  * 1: All major data types for the CBOR values must be as short as possible.
//        * Unsigned integer between 0 to 23 must be expressed in same byte as
//            the major type.
//        * 24 to 255 must be expressed only with an additional uint8_t.
//        * 256 to 65535 must be expressed only with an additional uint16_t.
//        * 65536 to 4294967295 must be expressed only with an additional
//            uint32_t. * The rules for expression of length in major types
//            2 to 5 follow the above rule for integers.
//  * 2: Keys in every map must be sorted (by major type, by key length,
//        by value in byte-wise lexical order).
//  * 3: Indefinite length items must be
//        converted to definite length items.
//  * 4: The depth of nested CBOR structures used by all CBOR encodings is
//        limited to at most 4 layers.
//  * 5: All maps must not have duplicate keys.
//
// Current implementation of CBORWriter encoder meets 1st, 2nd, 4th, and 5th
// requirements of canonical CBOR for CTAP.

enum class CborMajorType {
  kUnsigned = 0,    // Unsigned integer.
  kNegative = 1,    // Negative integer. Unsupported by this implementation.
  kByteString = 2,  // Byte string.
  kString = 3,      // String.
  kArray = 4,       // Array.
  kMap = 5,         // Map.
};

namespace content {

namespace {
// Maximum depth of nested CBOR.
constexpr uint8_t kMaxNestingLevel = 4;
// Mask selecting the last 5 bits  of the "initial byte" where
// 'additional information is encoded.
constexpr uint8_t kAdditionalInformationDataMask = 0x1F;
// Indicates the integer is in the following byte.
constexpr uint8_t kAdditionalInformation1Byte = 24;
// Indicates the integer is in the next 2 bytes.
constexpr uint8_t kAdditionalInformation2Bytes = 25;
// Indicates the integer is in the next 4 bytes.
constexpr uint8_t kAdditionalInformation4Bytes = 26;
// Indicates the integer is in the next 8 bytes.
constexpr uint8_t kAdditionalInformation8Bytes = 27;

}  // namespace

class CONTENT_EXPORT CBORWriter {
 public:
  ~CBORWriter();

  // Generates a CBOR byte string.
  static std::vector<uint8_t> Write(const CBORValue& node);

 private:
  CBORWriter(std::vector<uint8_t>* cbor);

  // Called recursively to build the CBOR bytestring. When completed,
  // |encoded_cbor_| will contain the CBOR.
  void EncodeCBOR(const CBORValue& node);

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
  std::vector<uint8_t>* encoded_cbor_;

  // Returns whether input CBOR value has at most 4 nested layers.
  // Nested CBOR is defined as any combination of CBOR maps and/or CBOR arrays.
  // Because some authenticators are memory constrained, the depth of nested
  // CBOR structures used by all message encodings is limited to at most four.
  //
  // For example, below CBOR format would have 3 layers of nesting.
  //     0xa2,  // CBORValue::MapValue with 2 keys.
  //       0x61, 0x61,  // key:"a", value:CBORValue(1)
  //       0x01,
  //
  //       0x61, 0x62,  // key:"b", value:CBORValue::MapValue with 2 keys.
  //       0xa2,
  //         0x61, 0x63,   // key:"c", value:value:CBORValue(2)
  //         0x02,
  //
  //         0x61, 0x64,   // key:"d", value:value:CBORValue(3)
  //         0x03,
  static bool IsValidDepth(const CBORValue& node);

  // Added to test private member function IsValidDepth()
  FRIEND_TEST_ALL_PREFIXES(CBORWriterTest, TestWriterIsValidDepthSingleLayer);
  FRIEND_TEST_ALL_PREFIXES(CBORWriterTest, TestWriterIsValidDepthMultiLayer);
  FRIEND_TEST_ALL_PREFIXES(CBORWriterTest,
                           TestWriterIsValidDepthUnbalancedCBOR);
  FRIEND_TEST_ALL_PREFIXES(CBORWriterTest,
                           TestWriterIsValidDepthOverlyNestedCBOR);

  DISALLOW_COPY_AND_ASSIGN(CBORWriter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_WRITER_H_

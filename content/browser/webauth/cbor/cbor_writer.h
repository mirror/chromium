// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_WRITER_H_
#define CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_WRITER_H_

#include <stdint.h>
#include <string>
#include <vector>
#include "base/numerics/safe_math.h"
#include "base/optional.h"
#include "base/strings/string_piece.h"
#include "content/browser/webauth/cbor/cbor_values.h"
#include "content/common/content_export.h"

#include "base/gtest_prod_util.h"

// A basic Concise Binary Object Representation (CBOR) encoder as defined by
// https://tools.ietf.org/html/rfc7049. This is a generic encoder that supplies
// canonical, well-formed CBOR values but does not guarantee their validity
// (see https://tools.ietf.org/html/rfc7049#section-3.2).
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
// Requirements for canonical CBOR as suggested by RFC 7049 are:
//  1) All major data types for the CBOR values must be as short as possible.
//      * Unsigned integer between 0 to 23 must be expressed in same byte as
//            the major type.
//      * 24 to 255 must be expressed only with an additional uint8_t.
//      * 256 to 65535 must be expressed only with an additional uint16_t.
//      * 65536 to 4294967295 must be expressed only with an additional
//            uint32_t. * The rules for expression of length in major types
//            2 to 5 follow the above rule for integers.
//  2) Keys in every map must be sorted (by major type, by key length, by value
//         in byte-wise lexical order).
//  3) Indefinite length items must be converted to definite length items.
//  4) All maps must not have duplicate keys.
//
// Current implementation of CBORWriter encoder meets all the requirements of
// canonical CBOR.

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
constexpr uint8_t kMaxNestingLevel = 16;
}  // namespace

class CONTENT_EXPORT CBORWriter {
 public:
  ~CBORWriter();

  // Returns an optional value of generated CBOR byte string. The return object
  // does not contain a value if the CBOR data being encoded does not meet the
  // maximum nesting layer size requirement. Nested CBOR is defined as any
  // combination of CBOR maps and/or CBOR arrays. Nesting level is defined by
  // number of hash- or arrayrefs that the encoder needs to traverse or the
  // number of { or [ characters without their matching closing parenthesis
  // crossed to reach a given character in a string. By default, maximum nesting
  // size is set to 16. Is is up to the caller or the application to determine
  // whether to add more strict nesting layer size constraints. For example,
  // for CTAP(Client To Authenticator Protocol), because some authenticators
  // are memory constrained, the depth of nested CBOR structures used by all
  // message encodings is limited to at most four.
  static base::Optional<std::vector<uint8_t>> Write(
      const CBORValue& node,
      uint8_t max_nesting_level = kMaxNestingLevel);

 private:
  CBORWriter(std::vector<uint8_t>* cbor);

  // Called recursively to build the CBOR bytestring. When completed,
  // |encoded_cbor_| will contain the CBOR.
  bool EncodeCBOR(const CBORValue& node);

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

  DISALLOW_COPY_AND_ASSIGN(CBORWriter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_WRITER_H_

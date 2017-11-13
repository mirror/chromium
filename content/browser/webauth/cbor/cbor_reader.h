// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_READER_H_
#define CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_READER_H_

#include "content/browser/webauth/cbor/cbor_binary.h"
#include "content/browser/webauth/cbor/cbor_values.h"

#include <vector>

#include "base/optional.h"
#include "content/common/content_export.h"

// Generic CBOR(Concise Binary Object Representation) decoder as defined by RFC
// 7049(https://tools.ietf.org/html/rfc7049). This decoder supports both
// non-canonical and canonical deserialization of CBOR data bytes.
// Supported:
//  * Major types:
//     * 0: Unsigned integers, up to 64-bit.
//     * 2: Byte strings.
//     * 3: UTF-8 strings.
//     * 4: Arrays, with the number of elements known at the start.
//     * 5: Maps, with the number of elements known at the start
//              of the container.
//
// Known limitations and interpretations of the RFC:
//  - Does not support negative integers, floating point numbers, indefinite
//    data streams and tagging.
//  - Assumes input for major type 2(string) and key for major type 5(map) are
//    encoded as UTF8.
//  - We limit nesting to 16 levels to prevent stack overflow. Callers of this
//    decoder can enforce stricter constraints on maximum nested depth size.
//  - Incomplete CBOR data items are treated as syntax errors.
// -  Trailing data bytes are not treated as errors but are ignored. Only the
//    first N bytes determined by the additional information bits are read.
//  - Unknown formats such as unknown additional information byte format are
//    treated as syntax errors.
//
// Callers of the decoder can determine whether or not to enforce canonical
// representations of CBOR data.
//
// Support for canonical CBOR representation:
//  - Duplicate keys for map are not allowed.
//  - Keys for map must be sorted in byte-wise lexical order.
//  - Trailing data bytes are not allowed.
//  - Support for stricter restrictions on nested layer depths.

namespace content {
namespace {
// CBOR nested depth sufficient for most use cases.
constexpr size_t kCBORMaxDepth = 16;
}  // namespace

class CONTENT_EXPORT CBORReader {
 public:
  enum CBORDecoderError {
    CBOR_NO_ERROR = 0,
    UNKNOWN_ADDITIONAL_INFO_ERROR,
    INCOMPLETE_CBOR_DATA_ERROR,
    INCORRECT_MAP_KEY_TYPE,
    TOO_MUCH_NESTING_ERROR,
    INVALID_UTF8_ERROR,
    EXTRANEOUS_DATA_ERROR,
    DUPLICATE_KEY_ERROR,
    OUT_OF_ORDER_KEY_ERROR,
  };

  // String versions of decoder error codes.
  static const char kUnknownAdditionalInfo[];
  static const char kIncompleteCBORData[];
  static const char kIncorrectMapKeyType[];
  static const char kTooMuchNesting[];
  static const char kInvalidUTF8[];
  static const char kExtraneousData[];
  static const char kDuplicateKey[];
  static const char kMapKeyOutOfOrder[];

  ~CBORReader();

  // Reads and parses |input_data| into a CBORValue. If any one of the syntax
  // formats is violated --including unknown additional info and incomplete
  // CBOR data --then an empty optional is returned. Optional |error_code_out|
  // can be provided by the caller to obtain additional information about
  // decoding failures. If |in_strict_mode| is set to true, then decoder
  // enforces canonical CBOR representations. Thus, if any one of syntax
  // formatting and/or canonical CBOR requirements are violated, then empty
  // optional is returned.
  static base::Optional<CBORValue> Read(
      const std::vector<uint8_t>& input_data,
      CBORDecoderError* error_code_out = nullptr,
      bool in_strict_mode = false,
      size_t max_nesing_level = kCBORMaxDepth);

  // Exposed interface that provides callers human readable error messages
  // for the provided |error_code|.
  static std::string ErrorCodeToString(CBORDecoderError error_code);

 private:
  CBORReader(std::vector<uint8_t>::const_iterator it,
             std::vector<uint8_t>::const_iterator end);
  base::Optional<CBORValue> DecodeCBOR(int max_nesting_level,
                                       bool in_strict_mode = false);
  bool ReadUnsignedInt(uint64_t* length, int additional_info);
  base::Optional<CBORValue> ReadBytes(uint64_t num_bytes);
  base::Optional<CBORValue> ReadString(uint64_t num_bytes);
  base::Optional<CBORValue> ReadCBORArray(uint64_t length,
                                          int max_nesting_level);
  base::Optional<CBORValue> ReadCBORMap(uint64_t length, int max_nesting_level);
  bool CanConsume(uint64_t bytes);
  void CheckExtraneousData();
  void CheckDuplicateKey(std::string new_key, CBORValue::MapValue* map);
  bool HasValidUTF8Format(std::string string_data);
  void CheckOutOfOrderKey(std::string new_key, CBORValue::MapValue* map);
  CBORDecoderError GetErrorCode();
  static std::string GetErrorMessage(CBORDecoderError error);
  std::vector<uint8_t>::const_iterator it_;
  std::vector<uint8_t>::const_iterator end_;
  std::vector<uint8_t> data_buffer_;
  CBORDecoderError error_code_;
  DISALLOW_COPY_AND_ASSIGN(CBORReader);
};
}  // namespace content
#endif  // CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_READER_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cbor/cbor_reader.h"

#include <math.h>

#include <utility>

#include "base/numerics/checked_math.h"
#include "base/numerics/safe_conversions.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "components/cbor/cbor_binary.h"

namespace cbor {

namespace {

CBORValue::Type GetMajorType(uint8_t initial_data_byte) {
  return static_cast<CBORValue::Type>(
      (initial_data_byte & constants::kMajorTypeMask) >>
      constants::kMajorTypeBitShift);
}

uint8_t GetAdditionalInfo(uint8_t initial_data_byte) {
  return initial_data_byte & constants::kAdditionalInformationMask;
}

// Error messages that correspond to each of the error codes.
const char kNoError[] = "Successfully deserialized to a CBOR value.";
const char kUnsupportedMajorType[] = "Unsupported major type.";
const char kUnknownAdditionalInfo[] =
    "Unknown additional info format in the first byte.";
const char kIncompleteCBORData[] =
    "Prematurely terminated CBOR data byte array.";
const char kIncorrectMapKeyType[] =
    "Specified map key type is not supported by the current implementation.";
const char kTooMuchNesting[] = "Too much nesting.";
const char kInvalidUTF8[] = "String encoding other than utf8 are not allowed.";
const char kExtraneousData[] = "Trailing data bytes are not allowed.";
const char kDuplicateKey[] = "Duplicate map keys are not allowed.";
const char kMapKeyOutOfOrder[] =
    "Map keys must be sorted by byte length and then by byte-wise lexical "
    "order.";
const char kNonMinimalCBOREncoding[] =
    "Unsigned integers must be encoded with minimum number of bytes.";
const char kUnsupportedSimpleValue[] =
    "Unsupported or unassigned simple value.";
const char kUnsupportedFloatingPointValue[] =
    "Floating point numbers are not supported.";
const char kOutOfRangeIntegerValue[] =
    "Integer values must be between INT64_MIN and INT64_MAX.";

}  // namespace

CBORReader::CBORReader(base::span<const uint8_t>::const_iterator it,
                       const base::span<const uint8_t>::const_iterator end)
    : it_(it), end_(end), error_code_(DecoderError::CBOR_NO_ERROR) {}
CBORReader::~CBORReader() {}

// static
base::Optional<CBORValue> CBORReader::Read(base::span<uint8_t const> data,
                                           DecoderError* error_code_out,
                                           int max_nesting_level) {
  CBORReader reader(data.cbegin(), data.cend());
  base::Optional<CBORValue> decoded_cbor = reader.DecodeCBOR(max_nesting_level);

  if (decoded_cbor)
    reader.CheckExtraneousData();
  if (error_code_out)
    *error_code_out = reader.GetErrorCode();

  if (reader.GetErrorCode() != DecoderError::CBOR_NO_ERROR)
    return base::nullopt;
  return decoded_cbor;
}

base::Optional<CBORValue> CBORReader::DecodeCBOR(int max_nesting_level) {
  if (max_nesting_level < 0 || max_nesting_level > kCBORMaxDepth) {
    error_code_ = DecoderError::TOO_MUCH_NESTING;
    return base::nullopt;
  }

  if (!CanConsume(1)) {
    error_code_ = DecoderError::INCOMPLETE_CBOR_DATA;
    return base::nullopt;
  }

  const uint8_t initial_byte = *it_++;
  const auto major_type = GetMajorType(initial_byte);
  const uint8_t additional_info = GetAdditionalInfo(initial_byte);

  uint64_t value;
  if (!ReadVariadicLengthInteger(additional_info, &value))
    return base::nullopt;

  switch (major_type) {
    case CBORValue::Type::UNSIGNED:
      return DecodeValueToUnsigned(value);
    case CBORValue::Type::NEGATIVE:
      return DecodeValueToNegative(value);
    case CBORValue::Type::BYTE_STRING:
      return ReadBytes(value);
    case CBORValue::Type::STRING:
      return ReadString(value);
    case CBORValue::Type::ARRAY:
      return ReadCBORArray(value, max_nesting_level);
    case CBORValue::Type::MAP:
      return ReadCBORMap(value, max_nesting_level);
    case CBORValue::Type::SIMPLE_VALUE:
      return ReadSimpleValue(additional_info, value);
    case CBORValue::Type::NONE:
      break;
  }

  error_code_ = DecoderError::UNSUPPORTED_MAJOR_TYPE;
  return base::nullopt;
}

bool CBORReader::ReadVariadicLengthInteger(uint8_t additional_info,
                                           uint64_t* value) {
  uint8_t additional_bytes = 0;
  if (additional_info < 24) {
    *value = additional_info;
    return true;
  } else if (additional_info == 24) {
    additional_bytes = 1;
  } else if (additional_info == 25) {
    additional_bytes = 2;
  } else if (additional_info == 26) {
    additional_bytes = 4;
  } else if (additional_info == 27) {
    additional_bytes = 8;
  } else {
    error_code_ = DecoderError::UNKNOWN_ADDITIONAL_INFO;
    return false;
  }

  if (!CanConsume(additional_bytes)) {
    error_code_ = DecoderError::INCOMPLETE_CBOR_DATA;
    return false;
  }

  uint64_t int_data = 0;
  for (uint8_t i = 0; i < additional_bytes; ++i) {
    int_data <<= 8;
    int_data |= *it_++;
  }

  *value = int_data;
  return CheckMinimalEncoding(additional_bytes, int_data);
}

base::Optional<CBORValue> CBORReader::DecodeValueToNegative(uint64_t value) {
  auto negative_value = -base::CheckedNumeric<int64_t>(value) - 1;
  if (!negative_value.IsValid()) {
    error_code_ = DecoderError::OUT_OF_RANGE_INTEGER_VALUE;
    return base::nullopt;
  }
  return CBORValue(negative_value.ValueOrDie());
}

base::Optional<CBORValue> CBORReader::DecodeValueToUnsigned(uint64_t value) {
  auto unsigned_value = base::CheckedNumeric<int64_t>(value);
  if (!unsigned_value.IsValid()) {
    error_code_ = DecoderError::OUT_OF_RANGE_INTEGER_VALUE;
    return base::nullopt;
  }
  return CBORValue(unsigned_value.ValueOrDie());
}

base::Optional<CBORValue> CBORReader::ReadSimpleValue(uint8_t additional_info,
                                                      uint64_t value) {
  // Floating point numbers are not supported.
  if (additional_info > 24 && additional_info < 28) {
    error_code_ = DecoderError::UNSUPPORTED_FLOATING_POINT_VALUE;
    return base::nullopt;
  }

  CHECK_LE(value, 255u);
  CBORValue::SimpleValue possibly_unsupported_simple_value =
      static_cast<CBORValue::SimpleValue>(static_cast<int>(value));
  switch (possibly_unsupported_simple_value) {
    case CBORValue::SimpleValue::FALSE_VALUE:
    case CBORValue::SimpleValue::TRUE_VALUE:
    case CBORValue::SimpleValue::NULL_VALUE:
    case CBORValue::SimpleValue::UNDEFINED:
      return CBORValue(possibly_unsupported_simple_value);
  }

  error_code_ = DecoderError::UNSUPPORTED_SIMPLE_VALUE;
  return base::nullopt;
}

base::Optional<CBORValue> CBORReader::ReadString(uint64_t num_bytes) {
  if (!CanConsume(num_bytes)) {
    error_code_ = DecoderError::INCOMPLETE_CBOR_DATA;
    return base::nullopt;
  }

  std::string cbor_string(it_, it_ + num_bytes);
  it_ += num_bytes;

  return HasValidUTF8Format(cbor_string)
             ? base::make_optional<CBORValue>(CBORValue(std::move(cbor_string)))
             : base::nullopt;
}

base::Optional<CBORValue> CBORReader::ReadBytes(uint64_t num_bytes) {
  if (!CanConsume(num_bytes)) {
    error_code_ = DecoderError::INCOMPLETE_CBOR_DATA;
    return base::nullopt;
  }

  std::vector<uint8_t> cbor_byte_string(it_, it_ + num_bytes);
  it_ += num_bytes;

  return CBORValue(std::move(cbor_byte_string));
}

base::Optional<CBORValue> CBORReader::ReadCBORArray(uint64_t length,
                                                    int max_nesting_level) {
  CBORValue::ArrayValue cbor_array;
  while (length-- > 0) {
    base::Optional<CBORValue> cbor_element = DecodeCBOR(max_nesting_level - 1);
    if (!cbor_element.has_value())
      return base::nullopt;
    cbor_array.push_back(std::move(cbor_element.value()));
  }
  return CBORValue(std::move(cbor_array));
}

base::Optional<CBORValue> CBORReader::ReadCBORMap(uint64_t length,
                                                  int max_nesting_level) {
  CBORValue::MapValue cbor_map;
  while (length-- > 0) {
    base::Optional<CBORValue> key = DecodeCBOR(max_nesting_level - 1);
    base::Optional<CBORValue> value = DecodeCBOR(max_nesting_level - 1);
    if (!key.has_value() || !value.has_value())
      return base::nullopt;

    switch (key.value().type()) {
      case CBORValue::Type::UNSIGNED:
      case CBORValue::Type::NEGATIVE:
      case CBORValue::Type::STRING:
      case CBORValue::Type::BYTE_STRING:
        break;
      default:
        error_code_ = DecoderError::INCORRECT_MAP_KEY_TYPE;
        return base::nullopt;
    }
    if (!CheckDuplicateKey(key.value(), &cbor_map) ||
        !CheckOutOfOrderKey(key.value(), &cbor_map)) {
      return base::nullopt;
    }

    cbor_map.insert_or_assign(std::move(key.value()), std::move(value.value()));
  }
  return CBORValue(std::move(cbor_map));
}

bool CBORReader::CanConsume(uint64_t bytes) {
  if (base::checked_cast<uint64_t>(std::distance(it_, end_)) >= bytes) {
    return true;
  }
  error_code_ = DecoderError::INCOMPLETE_CBOR_DATA;
  return false;
}

bool CBORReader::CheckMinimalEncoding(uint8_t additional_bytes,
                                      uint64_t uint_data) {
  if ((additional_bytes == 1 && uint_data < 24) ||
      uint_data <= (1ULL << 8 * (additional_bytes >> 1)) - 1) {
    error_code_ = DecoderError::NON_MINIMAL_CBOR_ENCODING;
    return false;
  }
  return true;
}

void CBORReader::CheckExtraneousData() {
  if (it_ != end_)
    error_code_ = DecoderError::EXTRANEOUS_DATA;
}

bool CBORReader::CheckDuplicateKey(const CBORValue& new_key,
                                   CBORValue::MapValue* map) {
  if (base::ContainsKey(*map, new_key)) {
    error_code_ = DecoderError::DUPLICATE_KEY;
    return false;
  }
  return true;
}

bool CBORReader::HasValidUTF8Format(const std::string& string_data) {
  if (!base::IsStringUTF8(string_data)) {
    error_code_ = DecoderError::INVALID_UTF8;
    return false;
  }
  return true;
}

bool CBORReader::CheckOutOfOrderKey(const CBORValue& new_key,
                                    CBORValue::MapValue* map) {
  auto comparator = map->key_comp();
  if (!map->empty() && comparator(new_key, map->rbegin()->first)) {
    error_code_ = DecoderError::OUT_OF_ORDER_KEY;
    return false;
  }
  return true;
}

CBORReader::DecoderError CBORReader::GetErrorCode() {
  return error_code_;
}

// static
const char* CBORReader::ErrorCodeToString(DecoderError error) {
  switch (error) {
    case DecoderError::CBOR_NO_ERROR:
      return kNoError;
    case DecoderError::UNSUPPORTED_MAJOR_TYPE:
      return kUnsupportedMajorType;
    case DecoderError::UNKNOWN_ADDITIONAL_INFO:
      return kUnknownAdditionalInfo;
    case DecoderError::INCOMPLETE_CBOR_DATA:
      return kIncompleteCBORData;
    case DecoderError::INCORRECT_MAP_KEY_TYPE:
      return kIncorrectMapKeyType;
    case DecoderError::TOO_MUCH_NESTING:
      return kTooMuchNesting;
    case DecoderError::INVALID_UTF8:
      return kInvalidUTF8;
    case DecoderError::EXTRANEOUS_DATA:
      return kExtraneousData;
    case DecoderError::DUPLICATE_KEY:
      return kDuplicateKey;
    case DecoderError::OUT_OF_ORDER_KEY:
      return kMapKeyOutOfOrder;
    case DecoderError::NON_MINIMAL_CBOR_ENCODING:
      return kNonMinimalCBOREncoding;
    case DecoderError::UNSUPPORTED_SIMPLE_VALUE:
      return kUnsupportedSimpleValue;
    case DecoderError::UNSUPPORTED_FLOATING_POINT_VALUE:
      return kUnsupportedFloatingPointValue;
    case DecoderError::OUT_OF_RANGE_INTEGER_VALUE:
      return kOutOfRangeIntegerValue;
    default:
      NOTREACHED();
      return "Unknown error code.";
  }
}

}  // namespace cbor

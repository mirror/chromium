// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/cbor/cbor_reader.h"
#include "content/browser/webauth/cbor/cbor_binary.h"

#include "base/numerics/safe_conversions.h"
#include "base/strings/string_util.h"

namespace content {

namespace {

uint8_t GetMajorType(uint8_t initial_data_byte) {
  return (initial_data_byte & authenticator_impl::kMajorTypeDataMask) >>
         authenticator_impl::kMajorTypeBitShift;
}

uint8_t GetAdditionalInfo(uint8_t initial_data_byte) {
  return initial_data_byte & authenticator_impl::kAdditionalInformationDataMask;
}

}  // namespace

// Error messages corresponding to each error codes.
const char CBORReader::kUnsupportedMajorType[] = "Unsupported major type.";
const char CBORReader::kUnknownAdditionalInfo[] =
    "Unknown additional info format in the first byte.";
const char CBORReader::kIncompleteCBORData[] =
    "Prematurely terminated CBOR data byte array.";
const char CBORReader::kIncorrectMapKeyType[] =
    "Map keys other than utf-8 encoded strings are not allowed.";
const char CBORReader::kTooMuchNesting[] = "Too much nesting.";
const char CBORReader::kInvalidUTF8[] =
    "String encoding other than utf8 for string type and CBOR map keys are not "
    "allowed for strict mode";
const char CBORReader::kExtraneousData[] =
    "Trailing data bytes allowed for strict mode";
const char CBORReader::kDuplicateKey[] =
    "Duplicate keys for CBOR map are not allowed in strict mode.";
const char CBORReader::kMapKeyOutOfOrder[] =
    "Keys for map has to be sorted for CBOR map in strict mode.";

CBORReader::CBORReader(std::vector<uint8_t>::const_iterator it,
                       std::vector<uint8_t>::const_iterator end)
    : it_(it), end_(end), error_code_(CBOR_NO_ERROR) {}
CBORReader::~CBORReader() {}

// static
base::Optional<CBORValue> CBORReader::Read(const std::vector<uint8_t>& data,
                                           CBORDecoderError* error_code_out,
                                           bool require_canonical_format,
                                           size_t max_nesting_level) {
  CBORReader reader(data.begin(), data.end());
  base::Optional<CBORValue> decoded_cbor =
      reader.DecodeCBOR(base::checked_cast<int>(max_nesting_level));

  if (decoded_cbor.has_value())
    reader.CheckExtraneousData();
  if (error_code_out)
    *error_code_out = reader.GetErrorCode();

  if (require_canonical_format && reader.GetErrorCode() != CBOR_NO_ERROR)
    return base::nullopt;
  return decoded_cbor;
}

base::Optional<CBORValue> CBORReader::DecodeCBOR(int max_nesting_level,
                                                 bool in_strict_mode) {
  if (max_nesting_level < 0) {
    error_code_ = TOO_MUCH_NESTING_ERROR;
    return base::nullopt;
  }

  if (!CanConsume(1))
    return base::nullopt;

  const uint8_t initial_byte = *it_++;
  const CBORValue::Type major_type =
      static_cast<CBORValue::Type>(GetMajorType(initial_byte));
  const uint8_t additional_info = GetAdditionalInfo(initial_byte);

  uint64_t length;
  if (!ReadUnsignedInt(additional_info, &length))
    return base::nullopt;

  switch (major_type) {
    case CBORValue::Type::UNSIGNED:
      return CBORValue(length);
    case CBORValue::Type::BYTE_STRING:
      return ReadBytes(length);
    case CBORValue::Type::STRING:
      return ReadString(length);
    case CBORValue::Type::ARRAY:
      return ReadCBORArray(length, max_nesting_level);
    case CBORValue::Type::MAP:
      return ReadCBORMap(length, max_nesting_level);
    default:
      break;
  }

  error_code_ = UNSUPPORTED_MAJOR_TYPE_ERROR;
  return base::nullopt;
}

bool CBORReader::ReadUnsignedInt(int additional_info, uint64_t* length) {
  uint8_t additional_bytes = 0;
  if (additional_info < 24) {
    *length = additional_info;
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
    error_code_ = UNKNOWN_ADDITIONAL_INFO_ERROR;
    return false;
  }

  uint64_t int_data = 0;
  while (additional_bytes-- > 0) {
    if (!CanConsume(1))
      return false;
    (int_data <<= 8) |= (*it_++ & 0xFFL);
  }

  *length = int_data;
  return true;
}

base::Optional<CBORValue> CBORReader::ReadString(uint64_t num_bytes) {
  if (!CanConsume(num_bytes))
    return base::nullopt;

  std::string cbor_string(num_bytes, '\0');
  std::transform(it_, it_ + num_bytes, cbor_string.begin(),
                 [](uint8_t v) { return static_cast<const char>(v); });
  it_ += num_bytes;

  return HasValidUTF8Format(cbor_string)
             ? base::make_optional(CBORValue(std::move(cbor_string)))
             : base::nullopt;
}

base::Optional<CBORValue> CBORReader::ReadBytes(uint64_t num_bytes) {
  if (!CanConsume(num_bytes))
    return base::nullopt;
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
    if (key.value().type() != CBORValue::Type::STRING) {
      error_code_ = INCORRECT_MAP_KEY_TYPE;
      return base::nullopt;
    }

    CheckDuplicateKey(key.value().GetString(), &cbor_map);
    CheckOutOfOrderKey(key.value().GetString(), &cbor_map);

    cbor_map.insert_or_assign(key.value().GetString(),
                              std::move(value.value()));
  }
  return CBORValue(std::move(cbor_map));
}

bool CBORReader::CanConsume(uint64_t bytes) {
  if (static_cast<uint64_t>(std::distance(it_, end_)) >= bytes) {
    return true;
  }
  error_code_ = INCOMPLETE_CBOR_DATA_ERROR;
  return false;
}

void CBORReader::CheckExtraneousData() {
  if (it_ != end_)
    error_code_ = EXTRANEOUS_DATA_ERROR;
}

void CBORReader::CheckDuplicateKey(std::string new_key,
                                   CBORValue::MapValue* map) {
  if (map->count(new_key) > 0)
    error_code_ = DUPLICATE_KEY_ERROR;
}

bool CBORReader::HasValidUTF8Format(std::string string_data) {
  if (!base::IsStringUTF8(string_data)) {
    error_code_ = INVALID_UTF8_ERROR;
    return false;
  }
  return true;
}

void CBORReader::CheckOutOfOrderKey(std::string new_key,
                                    CBORValue::MapValue* map) {
  auto comparator = map->key_comp();
  if (!map->empty() && comparator(new_key, map->rbegin()->first))
    error_code_ = OUT_OF_ORDER_KEY_ERROR;
}

CBORReader::CBORDecoderError CBORReader::GetErrorCode() {
  return error_code_;
}

// static
std::string CBORReader::ErrorCodeToString(CBORDecoderError error) {
  switch (error) {
    case CBOR_NO_ERROR:
      return std::string();
    case UNSUPPORTED_MAJOR_TYPE_ERROR:
      return kUnsupportedMajorType;
    case UNKNOWN_ADDITIONAL_INFO_ERROR:
      return kUnknownAdditionalInfo;
    case INCOMPLETE_CBOR_DATA_ERROR:
      return kIncompleteCBORData;
    case INCORRECT_MAP_KEY_TYPE:
      return kIncorrectMapKeyType;
    case TOO_MUCH_NESTING_ERROR:
      return kTooMuchNesting;
    case INVALID_UTF8_ERROR:
      return kInvalidUTF8;
    case EXTRANEOUS_DATA_ERROR:
      return kExtraneousData;
    case DUPLICATE_KEY_ERROR:
      return kDuplicateKey;
    case OUT_OF_ORDER_KEY_ERROR:
      return kMapKeyOutOfOrder;
    default:
      NOTREACHED();
      return std::string();
  }
}

}  // namespace content

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/cbor/cbor_reader.h"

#include "base/numerics/safe_conversions.h"
#include "base/strings/string_util.h"

namespace content {

// Error messages corresponding to each error codes.
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
                                           bool in_strict_mode,
                                           size_t max_nesting_level) {
  CBORReader reader(data.begin(), data.end());
  base::Optional<CBORValue> decoded_cbor =
      reader.DecodeCBOR(base::checked_cast<int>(max_nesting_level));

  if (decoded_cbor.has_value())
    reader.CheckExtraneousData();
  if (error_code_out)
    *error_code_out = reader.GetErrorCode();

  if (in_strict_mode && reader.GetErrorCode() != CBOR_NO_ERROR)
    return base::nullopt;
  return decoded_cbor;
}

base::Optional<CBORValue> CBORReader::DecodeCBOR(int max_nesting_level,
                                                 bool in_strict_mode) {
  if (max_nesting_level < 0) {
    error_code_ = TOO_MUCH_NESTING_ERROR;
    return base::nullopt;
  }

  if (!IteratorBoundCheck(1))
    return base::nullopt;

  FirstByte meda_data(*it_++);
  uint64_t length;
  if (!ReadLength(&length, meda_data.GetAdditionalInfo()))
    return base::nullopt;

  switch (meda_data.GetMajorType()) {
    case 0:
      return CBORValue(length);
    case 1:
      break;
    case 2: {
      std::vector<uint8_t> byte_data;
      if (ReadBytes(&byte_data, length))
        return CBORValue(byte_data);
      return base::nullopt;
    }
    case 3: {
      std::vector<char> buffer;
      if (ReadString(&buffer, length))
        return CBORValue(std::string(buffer.begin(), buffer.end()));
      return base::nullopt;
    }
    case 4: {
      CBORValue::ArrayValue cbor_array;
      if (ReadCBORArray(&cbor_array, length, max_nesting_level))
        return CBORValue(cbor_array);
      return base::nullopt;
    }
    case 5: {
      CBORValue::MapValue cbor_map;
      if (ReadCBORMap(&cbor_map, length, max_nesting_level))
        return CBORValue(cbor_map);
      return base::nullopt;
    }
    case 6:
    case 7:
    default:
      break;
  }
  return base::nullopt;
}

bool CBORReader::ReadInteger(uint64_t* length_data, uint64_t num_bytes) {
  uint64_t int_data = 0;
  while (num_bytes > 0) {
    int bit_shift = (num_bytes - 1) * 8;
    if (!IteratorBoundCheck(1))
      return false;
    int_data |= (*it_++ & 0xFFL) << bit_shift;
    num_bytes--;
  }
  *length_data = int_data;
  return true;
}

bool CBORReader::ReadString(std::vector<char>* buffer, uint64_t num_bytes) {
  while (num_bytes-- > 0) {
    if (!IteratorBoundCheck(1))
      return false;
    buffer->push_back(static_cast<const char>(*it_++));
  }
  return HasValidUTF8Format(std::string(buffer->begin(), buffer->end()));
}

bool CBORReader::ReadBytes(std::vector<uint8_t>* buffer, uint64_t num_bytes) {
  if (num_bytes > 0) {
    if (!IteratorBoundCheck(num_bytes))
      return false;
    buffer->insert(buffer->end(), it_, it_ + num_bytes);
    it_ += num_bytes;
  }
  return true;
}

bool CBORReader::ReadCBORArray(CBORValue::ArrayValue* array,
                               uint64_t num_bytes,
                               int max_nesting_level) {
  while (num_bytes-- > 0) {
    base::Optional<CBORValue> cbor_element = DecodeCBOR(max_nesting_level - 1);
    if (!cbor_element.has_value())
      return false;
    array->push_back(std::move(cbor_element.value()));
  }
  return true;
}

bool CBORReader::ReadCBORMap(CBORValue::MapValue* map,
                             uint64_t num_bytes,
                             int max_nesting_level) {
  while (num_bytes-- > 0) {
    base::Optional<CBORValue> key = DecodeCBOR(max_nesting_level - 1);
    base::Optional<CBORValue> val = DecodeCBOR(max_nesting_level - 1);
    if (!key.has_value() || !val.has_value())
      return false;
    if (key.value().type() != CBORValue::Type::STRING) {
      error_code_ = INCORRECT_MAP_KEY_TYPE;
      return false;
    }

    CheckDuplicateKey(key.value().GetString(), map);
    CheckOutOfOrderKey(key.value().GetString(), map);

    map->insert_or_assign(key.value().GetString(), std::move(val.value()));
  }
  return true;
}

bool CBORReader::ReadLength(uint64_t* length, int additional_info) {
  if (additional_info < 24) {
    *length = additional_info;
    return true;
  } else if (additional_info == 24) {
    return ReadInteger(length, 1);
  } else if (additional_info == 25) {
    return ReadInteger(length, 2);
  } else if (additional_info == 26) {
    return ReadInteger(length, 4);
  } else if (additional_info == 27) {
    return ReadInteger(length, 8);
  }
  error_code_ = UNKNOWN_ADDITIONAL_INFO_ERROR;
  return false;
}

bool CBORReader::IteratorBoundCheck(uint64_t step) {
  if (static_cast<uint64_t>(std::distance(it_, end_)) >= step) {
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

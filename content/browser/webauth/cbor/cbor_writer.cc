// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/cbor/cbor_writer.h"

#include <stdint.h>

#include "base/numerics/safe_conversions.h"
#include "base/values.h"

namespace content {

CBORWriter::~CBORWriter(){};

// static
void CBORWriter::Write(const base::Value& node, std::vector<uint8_t>* cbor) {
  cbor->clear();
  CBORWriter writer(cbor);
  writer.EncodeCBOR(node);
}

CBORWriter::CBORWriter(std::vector<uint8_t>* cbor) : encoded_cbor_(cbor) {
  DCHECK(cbor->empty());
}

void CBORWriter::EncodeCBOR(const base::Value& node) {
  switch (node.GetType()) {
    case base::Value::Type::NONE: {
      NOTREACHED();
      return;
    }

    // Represents unsigned integers.
    case base::Value::Type::DOUBLE: {
      double value = node.GetDouble();
      StartItem(CborMajorType::kUnsigned, value);
      return;
    }

    // Represents a byte string.
    case base::Value::Type::BINARY: {
      const base::Value::BlobStorage& bytes = node.GetBlob();
      ;
      StartItem(CborMajorType::kByteString,
                base::strict_cast<uint64_t>(bytes.size()));
      // Add the bytes.
      encoded_cbor_->insert(encoded_cbor_->end(), bytes.begin(), bytes.end());
      return;
    }

    case base::Value::Type::STRING: {
      base::StringPiece string = node.GetString();
      StartItem(CborMajorType::kString,
                base::strict_cast<uint64_t>(string.size()));

      // Add the characters.
      encoded_cbor_->insert(encoded_cbor_->end(), string.begin(), string.end());
      return;
    }

    // Represents an array.
    case base::Value::Type::LIST: {
      const base::Value::ListStorage& list = node.GetList();
      StartItem(CborMajorType::kArray, list.size());
      for (const auto& value : list) {
        EncodeCBOR(value);
      }
      return;
    }

    // Represents a map.
    case base::Value::Type::DICTIONARY: {
      const base::DictionaryValue* dict;
      node.GetAsDictionary(&dict);
      StartItem(CborMajorType::kMap, dict->size());
      for (base::DictionaryValue::Iterator itr(*dict); !itr.IsAtEnd();
           itr.Advance()) {
        EncodeCBOR(base::Value(itr.key()));
        EncodeCBOR(itr.value());
      }
      return;
    }

    case base::Value::Type::BOOLEAN: {
      NOTREACHED();
      return;
    }

    // INTEGER in this case represents signed values.
    case base::Value::Type::INTEGER: {
      NOTREACHED();
      return;
    }
  }
  NOTREACHED();
  return;
}

void CBORWriter::StartItem(CborMajorType type, uint64_t size) {
  encoded_cbor_->push_back(base::checked_cast<uint8_t>(type) << 5);
  SetUint(size);
}

void CBORWriter::SetAdditionalInformation(uint8_t additional_information) {
  encoded_cbor_->at(encoded_cbor_->size() - 1) |=
      (additional_information & kAdditionalInformationDataMask);
}

void CBORWriter::SetUint(uint64_t value) {
  size_t count = GetNumUintBytes(value);
  int shift = -1;
  // Values under 24 are encoded directly in the initial byte.
  // Otherwise, the last 5 bits of the initial byte contains the length
  // of unsigned integer, which is encoded in following bytes.
  switch (count) {
    case 0:
      SetAdditionalInformation(base::checked_cast<uint8_t>(value));
      break;
    case 1:
      SetAdditionalInformation(kAdditionalInformation1Byte);
      shift = 0;
      break;
    case 2:
      SetAdditionalInformation(kAdditionalInformation2Bytes);
      shift = 1;
      break;
      break;
    case 4:
      SetAdditionalInformation(kAdditionalInformation4Bytes);
      shift = 3;
      break;
      return;
    case 8:
      SetAdditionalInformation(kAdditionalInformation8Bytes);
      shift = 7;
      break;
  }
  for (; shift >= 0; shift--) {
    encoded_cbor_->push_back(
        base::checked_cast<uint8_t>(0xFF & (value >> (shift * 8))));
  }
  return;
}

size_t CBORWriter::GetNumUintBytes(uint64_t value) {
  if (value < 24) {
    return 0;
  } else if (value <= 0xFF) {
    return 1;
  } else if (value <= 0xFFFF) {
    return 2;
  } else if (value <= 0xFFFFFFFF) {
    return 4;
  }
  return 8;
}

base::Value CBORWriter::Unsigned(uint64_t value) {
  return base::Value(static_cast<double>(value));
};

base::Value CBORWriter::Bytestring(std::vector<char> bytes) {
  return base::Value(bytes);
};

base::Value CBORWriter::UTF8String(base::StringPiece string) {
  return base::Value(string);
};

}  // namespace content

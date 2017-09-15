// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/protocol_string.h"

#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/browser/devtools/protocol/protocol.h"

namespace content {
namespace protocol {

namespace {

inline bool EscapeChar(uint16_t c, StringBuilder& dst) {
  switch (c) {
    case '\b':
      StringUtil::builderAppend(dst, "\\b");
      break;
    case '\f':
      StringUtil::builderAppend(dst, "\\f");
      break;
    case '\n':
      StringUtil::builderAppend(dst, "\\n");
      break;
    case '\r':
      StringUtil::builderAppend(dst, "\\r");
      break;
    case '\t':
      StringUtil::builderAppend(dst, "\\t");
      break;
    case '\\':
      StringUtil::builderAppend(dst, "\\\\");
      break;
    case '"':
      StringUtil::builderAppend(dst, "\\\"");
      break;
    default:
      return false;
  }
  return true;
}

const char hex_digits[17] = "0123456789ABCDEF";

}  // anonymous namespace

std::unique_ptr<protocol::Value> toProtocolValue(
    const base::Value* value, int depth) {
  if (!value || !depth)
    return nullptr;
  if (value->IsType(base::Value::Type::NONE))
    return protocol::Value::null();
  if (value->IsType(base::Value::Type::BOOLEAN)) {
    bool inner;
    value->GetAsBoolean(&inner);
    return protocol::FundamentalValue::create(inner);
  }
  if (value->IsType(base::Value::Type::INTEGER)) {
    int inner;
    value->GetAsInteger(&inner);
    return protocol::FundamentalValue::create(inner);
  }
  if (value->IsType(base::Value::Type::DOUBLE)) {
    double inner;
    value->GetAsDouble(&inner);
    return protocol::FundamentalValue::create(inner);
  }
  if (value->IsType(base::Value::Type::STRING)) {
    std::string inner;
    value->GetAsString(&inner);
    return protocol::StringValue::create(inner);
  }
  if (value->IsType(base::Value::Type::LIST)) {
    const base::ListValue* list = nullptr;
    value->GetAsList(&list);
    std::unique_ptr<protocol::ListValue> result = protocol::ListValue::create();
    for (size_t i = 0; i < list->GetSize(); i++) {
      const base::Value* item = nullptr;
      list->Get(i, &item);
      std::unique_ptr<protocol::Value> converted =
          toProtocolValue(item, depth - 1);
      if (converted)
        result->pushValue(std::move(converted));
    }
    return std::move(result);
  }
  if (value->IsType(base::Value::Type::DICTIONARY)) {
    const base::DictionaryValue* dictionary = nullptr;
    value->GetAsDictionary(&dictionary);
    std::unique_ptr<protocol::DictionaryValue> result =
        protocol::DictionaryValue::create();
    for (base::DictionaryValue::Iterator it(*dictionary);
        !it.IsAtEnd(); it.Advance()) {
      std::unique_ptr<protocol::Value> converted =
          toProtocolValue(&it.value(), depth - 1);
      if (converted)
        result->setValue(it.key(), std::move(converted));
    }
    return std::move(result);
  }
  return nullptr;
}

std::unique_ptr<base::Value> toBaseValue(
    protocol::Value* value, int depth) {
  if (!value || !depth)
    return nullptr;
  if (value->type() == protocol::Value::TypeNull)
    return base::MakeUnique<base::Value>();
  if (value->type() == protocol::Value::TypeBoolean) {
    bool inner;
    value->asBoolean(&inner);
    return base::WrapUnique(new base::Value(inner));
  }
  if (value->type() == protocol::Value::TypeInteger) {
    int inner;
    value->asInteger(&inner);
    return base::WrapUnique(new base::Value(inner));
  }
  if (value->type() == protocol::Value::TypeDouble) {
    double inner;
    value->asDouble(&inner);
    return base::WrapUnique(new base::Value(inner));
  }
  if (value->type() == protocol::Value::TypeString) {
    std::string inner;
    value->asString(&inner);
    return base::WrapUnique(new base::Value(inner));
  }
  if (value->type() == protocol::Value::TypeArray) {
    protocol::ListValue* list = protocol::ListValue::cast(value);
    std::unique_ptr<base::ListValue> result(new base::ListValue());
    for (size_t i = 0; i < list->size(); i++) {
      std::unique_ptr<base::Value> converted =
          toBaseValue(list->at(i), depth - 1);
      if (converted)
        result->Append(std::move(converted));
    }
    return std::move(result);
  }
  if (value->type() == protocol::Value::TypeObject) {
    protocol::DictionaryValue* dict = protocol::DictionaryValue::cast(value);
    std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());
    for (size_t i = 0; i < dict->size(); i++) {
      protocol::DictionaryValue::Entry entry = dict->at(i);
      std::unique_ptr<base::Value> converted =
          toBaseValue(entry.second, depth - 1);
      if (converted)
        result->SetWithoutPathExpansion(entry.first, std::move(converted));
    }
    return std::move(result);
  }
  return nullptr;
}

// static
std::unique_ptr<protocol::Value> StringUtil::parseJSON(
    const std::string& json) {
  std::unique_ptr<base::Value> value = base::JSONReader::Read(json);
  return toProtocolValue(value.get(), 1000);
}

StringBuilder::StringBuilder() {}

StringBuilder::~StringBuilder() {}

void StringBuilder::append(const std::string& s) {
  string_ += s;
}

void StringBuilder::append(char c) {
  string_ += c;
}

void StringBuilder::append(const char* characters, size_t length) {
  string_.append(characters, length);
}

// static
void StringUtil::builderAppendQuotedString(StringBuilder& builder,
                                           const String& str) {
  std::string utf8char;
  utf8char.reserve(4);

  builder.append('"');
  base::string16 str16 = base::UTF8ToUTF16(str);
  for (unsigned i = 0; i < str16.length(); ++i) {
    base::char16 c = str16[i];
    if (!EscapeChar(c, builder)) {
      if (c < 32 || c > 126 || c == '<' || c == '>') {
        // 1. Escaping <, > to prevent script execution.
        // 2. Technically, we could also pass through c > 126 as UTF8, but this
        //    is also optional. It would also be a pain to implement here.
        StringUtil::builderAppend(builder, "\\u");
        uint16_t number = c;
        for (size_t j = 0; j < 4; ++j) {
          char digit = hex_digits[(number & 0xF000) >> 12];
          StringUtil::builderAppend(builder, digit);
          number <<= 4;
        }
      } else {
        base::UTF16ToUTF8(&c, 1, &utf8char);
        builder.append(utf8char);
      }
    }
  }
  builder.append('"');
}

std::string StringBuilder::toString() {
  return string_;
}

void StringBuilder::reserveCapacity(size_t capacity) {
  string_.reserve(capacity);
}

}  // namespace protocol
}  // namespace content

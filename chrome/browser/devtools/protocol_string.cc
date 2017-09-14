// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/protocol_string.h"

#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "chrome/browser/devtools/protocol/protocol.h"

namespace protocol {

std::unique_ptr<protocol::Value> toProtocolValue(const base::Value* value,
                                                 int depth) {
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
    for (base::DictionaryValue::Iterator it(*dictionary); !it.IsAtEnd();
         it.Advance()) {
      std::unique_ptr<protocol::Value> converted =
          toProtocolValue(&it.value(), depth - 1);
      if (converted)
        result->setValue(it.key(), std::move(converted));
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

std::string StringBuilder::toString() {
  return string_;
}

void StringBuilder::reserveCapacity(size_t capacity) {
  string_.reserve(capacity);
}

}  // namespace protocol

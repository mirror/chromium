// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/base/json_message.h"

#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"

namespace chromecast {

JsonMessage::JsonMessage()
    : dictionary_(new base::DictionaryValue()),
      valid_(true) {
}

JsonMessage::JsonMessage(scoped_ptr<base::DictionaryValue> dictionary)
    : dictionary_(dictionary.Pass()),
      valid_(true) {
}

JsonMessage::JsonMessage(const std::string& json)
    : dictionary_(new base::DictionaryValue()),
      valid_(true) {
  JSONStringValueDeserializer serializer(json);
  int json_error_code = 0;
  std::string json_error_str;
  scoped_ptr<base::Value> value(serializer.Deserialize(&json_error_code,
                                                       &json_error_str));
  if (value && value->GetAsDictionary(nullptr)) {
    // Don't use DictionaryValue::Swap() since it is not created by
    // JSON_DETACHABLE_CHILDREN option.
    dictionary_.swap(
        *(reinterpret_cast<scoped_ptr<base::DictionaryValue>*>(&value)));
  } else {
    VLOG(1) << "Json deserialization error: code=" << json_error_code
            << ", message=" << json_error_str;
    valid_ = false;
  }
}

JsonMessage::~JsonMessage() {
}

base::Value* JsonMessage::Release() {
  return dictionary_.release();
}

const base::Value* JsonMessage::Get() const {
  return dictionary_.get();
}

void JsonMessage::GetSubDictionary(const std::string& key,
                                   base::DictionaryValue** out_value) {
  if (!dictionary_->GetDictionary(key, out_value)) {
    *out_value = new base::DictionaryValue;
    dictionary_->Set(key, *out_value);
  }
}

void JsonMessage::GetSubList(const std::string& key,
                             base::ListValue** out_value) {
  if (!dictionary_->GetList(key, out_value)) {
    *out_value = new base::ListValue;
    dictionary_->Set(key, *out_value);
  }
}

bool JsonMessage::ToJson(std::string* json) const {
  JSONStringValueSerializer serializer(json);
  return serializer.Serialize(*dictionary_);
}

}  // namespace chromecast

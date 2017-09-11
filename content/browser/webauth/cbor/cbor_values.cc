// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/cbor/cbor_values.h"

#include <string.h>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"

namespace content {

CBORValue::CBORValue() noexcept : type_(Type::NONE) {}

CBORValue::CBORValue(CBORValue&& that) noexcept {
  InternalMoveConstructFrom(std::move(that));
}

CBORValue::CBORValue(Type type) : type_(type) {
  // Initialize with the default value.
  switch (type_) {
    case Type::NONE:
      return;
    case Type::UNSIGNED:
      unsigned_value_ = 0;
      return;
    case Type::BYTESTRING:
      bytestring_value_.Init();
      return;
    case Type::STRING:
      string_value_.Init();
      return;
    case Type::ARRAY:
      array_value_.Init();
      return;
    case Type::MAP:
      map_value_.Init();
      return;
  }
}

CBORValue::CBORValue(uint64_t in_unsigned)
    : type_(Type::UNSIGNED), unsigned_value_(in_unsigned) {}

CBORValue::CBORValue(const std::vector<uint8_t>& in_bytes)
    : type_(Type::BYTESTRING) {
  bytestring_value_.Init(in_bytes);
}

CBORValue::CBORValue(std::vector<uint8_t>&& in_bytes) noexcept
    : type_(Type::BYTESTRING) {
  bytestring_value_.Init(std::move(in_bytes));
}

CBORValue::CBORValue(const char* in_string) : type_(Type::STRING) {
  string_value_.Init(in_string);
  DCHECK(base::IsStringUTF8(*string_value_));
}

CBORValue::CBORValue(const std::string& in_string) : type_(Type::STRING) {
  string_value_.Init(in_string);
  DCHECK(base::IsStringUTF8(*string_value_));
}

CBORValue::CBORValue(std::string&& in_string) noexcept : type_(Type::STRING) {
  string_value_.Init(std::move(in_string));
  DCHECK(base::IsStringUTF8(*string_value_));
}

CBORValue::CBORValue(base::StringPiece in_string)
    : CBORValue(in_string.as_string()) {}

CBORValue::CBORValue(const std::vector<CBORValue>& in_array)
    : type_(Type::ARRAY) {
  array_value_.Init();
  array_value_->reserve(in_array.size());
  for (const auto& val : in_array)
    array_value_->emplace_back(val.Clone());
}

CBORValue::CBORValue(std::vector<CBORValue>&& in_array) noexcept
    : type_(Type::ARRAY) {
  array_value_.Init(std::move(in_array));
}

CBORValue::CBORValue(const base::flat_map<std::string, CBORValue>& in_map)
    : type_(Type::MAP) {
  map_value_.Init();
  map_value_->reserve(in_map.size());
  for (const auto& it : in_map) {
    map_value_->emplace(it.first, it.second.Clone());
  }
}

CBORValue::CBORValue(base::flat_map<std::string, CBORValue>&& in_map) noexcept
    : type_(Type::MAP) {
  map_value_.Init(std::move(in_map));
}

CBORValue& CBORValue::operator=(CBORValue&& that) noexcept {
  InternalCleanup();
  InternalMoveConstructFrom(std::move(that));

  return *this;
}

CBORValue::~CBORValue() {
  InternalCleanup();
}

uint64_t CBORValue::GetUnsigned() const {
  CHECK(is_unsigned());
  return unsigned_value_;
}

const std::string& CBORValue::GetString() const {
  CHECK(is_string());
  return *string_value_;
}

const std::vector<uint8_t>& CBORValue::GetBytestring() const {
  CHECK(is_bytestring());
  return *bytestring_value_;
}

const std::vector<CBORValue>& CBORValue::GetArray() const {
  CHECK(is_array());
  return *array_value_;
}

const base::flat_map<std::string, CBORValue>& CBORValue::GetMap() const {
  CHECK(is_map());
  return *map_value_;
}

CBORValue CBORValue::Clone() const {
  switch (type_) {
    case Type::NONE:
      return CBORValue();
    case Type::UNSIGNED:
      return CBORValue(unsigned_value_);
    case Type::BYTESTRING:
      return CBORValue(*bytestring_value_);
    case Type::STRING:
      return CBORValue(*string_value_);
    case Type::ARRAY:
      return CBORValue(*array_value_);
    case Type::MAP:
      return CBORValue(*map_value_);
  }

  NOTREACHED();
  return CBORValue();
}

void CBORValue::InternalMoveConstructFrom(CBORValue&& that) {
  type_ = that.type_;

  switch (type_) {
    case Type::NONE:
      return;
    case Type::UNSIGNED:
      unsigned_value_ = that.unsigned_value_;
      return;
    case Type::BYTESTRING:
      bytestring_value_.InitFromMove(std::move(that.bytestring_value_));
      return;
    case Type::STRING:
      string_value_.InitFromMove(std::move(that.string_value_));
      return;
    case Type::ARRAY:
      array_value_.InitFromMove(std::move(that.array_value_));
      return;
    case Type::MAP:
      map_value_.InitFromMove(std::move(that.map_value_));
      return;
  }
}

void CBORValue::InternalCleanup() {
  switch (type_) {
    case Type::NONE:
    case Type::UNSIGNED:
      // Nothing to do
      return;
    case Type::BYTESTRING:
      bytestring_value_.Destroy();
      return;
    case Type::STRING:
      string_value_.Destroy();
      return;
    case Type::ARRAY:
      array_value_.Destroy();
      return;
    case Type::MAP:
      map_value_.Destroy();
      return;
  }
}

}  // namespace content
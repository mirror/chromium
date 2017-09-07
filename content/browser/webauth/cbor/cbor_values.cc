// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/cbor/cbor_values.h"

#include <string.h>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"

namespace content {

CBORValue::CBORValue() : type_(Type::NONE) {}

CBORValue::CBORValue(CBORValue&& that) {
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

CBORValue::CBORValue(base::StringPiece in_string) : type_(Type::STRING) {
  string_value_.Init(in_string);
  DCHECK(base::IsStringUTF8(in_string.as_string()));
}

CBORValue::CBORValue(const std::vector<uint8_t>& in_bytes)
    : type_(Type::BYTESTRING) {
  bytestring_value_.Init(in_bytes);
}

CBORValue::CBORValue(const std::vector<CBORValue>& in_array)
    : type_(Type::ARRAY) {
  array_value_.Init();
  array_value_->reserve(in_array.size());
  for (const auto& val : in_array)
    array_value_->emplace_back(val.Clone());
}

CBORValue::CBORValue(
    const base::flat_map<std::string, std::unique_ptr<CBORValue>>& in_map)
    : type_(Type::MAP) {
  map_value_.Init();
  map_value_->reserve(in_map.size());
  for (const auto& it : in_map) {
    map_value_->emplace_hint(map_value_->end(), it.first,
                             base::MakeUnique<CBORValue>(it.second->Clone()));
  }
}

CBORValue::~CBORValue() {
  InternalCleanup();
}

uint64_t CBORValue::GetUnsigned() const {
  CHECK(isUnsigned());
  return unsigned_value_;
}

const base::StringPiece& CBORValue::GetString() const {
  CHECK(isString());
  return *string_value_;
}

const std::vector<uint8_t>& CBORValue::GetBytestring() const {
  CHECK(isBytestring());
  return *bytestring_value_;
}

const std::vector<CBORValue>& CBORValue::GetArray() const {
  CHECK(isArray());
  return *array_value_;
}

const base::flat_map<std::string, std::unique_ptr<CBORValue>>&
CBORValue::GetMap() const {
  CHECK(isMap());
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

MapValue::MapValue() : CBORValue(Type::MAP) {}
MapValue::MapValue(
    const base::flat_map<std::string, std::unique_ptr<CBORValue>>& in_map)
    : CBORValue(in_map) {}

MapValue::~MapValue() {}

void MapValue::Insert(std::string key, base::StringPiece string) {
  map_value_->emplace_hint(map_value_->end(), key,
                           base::MakeUnique<CBORValue>(CBORValue(string)));
}

void MapValue::Insert(std::string key, uint64_t value) {
  map_value_->emplace_hint(map_value_->end(), key,
                           base::MakeUnique<CBORValue>(CBORValue(value)));
}

void MapValue::Insert(std::string key, std::vector<uint8_t>& bytestring) {
  map_value_->emplace_hint(map_value_->end(), key,
                           base::MakeUnique<CBORValue>(CBORValue(bytestring)));
}

void MapValue::Insert(std::string key, std::vector<CBORValue>& array) {
  map_value_->emplace_hint(map_value_->end(), key,
                           base::MakeUnique<CBORValue>(CBORValue(array)));
}

void MapValue::Insert(
    std::string key,
    base::flat_map<std::string, std::unique_ptr<CBORValue>>& map) {
  map_value_->emplace_hint(map_value_->end(), key,
                           base::MakeUnique<CBORValue>(CBORValue(map)));
}

}  // namespace content
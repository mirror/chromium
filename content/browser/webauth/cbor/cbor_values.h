// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_VALUES_H_
#define CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_VALUES_H_

#include <stdint.h>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/manual_constructor.h"
#include "base/strings/string_piece.h"
#include "content/common/content_export.h"

namespace content {

class MapValue;
class CBORValue;

// A class for Concise Binary Object Representation (CBOR) values.
// This does not support:
//  * Negative integers.
//  * Floating-point numbers.
//  * Indefinite-length encodings.
class CONTENT_EXPORT CBORValue {
 public:
  enum class Type {
    NONE,
    UNSIGNED,
    BYTESTRING,
    STRING,
    ARRAY,
    MAP,
  };

  CBORValue(CBORValue&& that);
  CBORValue();  // A null value.

  // CBORValue's copy constructor and copy assignment operator are deleted.
  // Use this to obtain a deep copy explicitly.
  CBORValue Clone() const;

  explicit CBORValue(Type type);
  explicit CBORValue(uint64_t in_unsigned);
  explicit CBORValue(const std::vector<uint8_t>& in_bytes);
  explicit CBORValue(base::StringPiece in_string);
  explicit CBORValue(const std::vector<CBORValue>& in_array);
  explicit CBORValue(
      const base::flat_map<std::string, std::unique_ptr<CBORValue>>& in_map);

  ~CBORValue();

  // Returns the type of the value stored by the current Value object.
  Type type() const { return type_; }

  // Returns true if the current object represents a given type.
  bool IsType(Type type) const { return type == type_; }
  bool isNone() const { return type() == Type::NONE; }
  bool isUnsigned() const { return type() == Type::UNSIGNED; }
  bool isBytestring() const { return type() == Type::BYTESTRING; }
  bool isString() const { return type() == Type::STRING; }
  bool isArray() const { return type() == Type::ARRAY; }
  bool isMap() const { return type() == Type::MAP; }

  // These will all fatally assert if the type doesn't match.
  uint64_t GetUnsigned() const;
  const base::StringPiece& GetString() const;
  const std::vector<uint8_t>& GetBytestring() const;
  const std::vector<CBORValue>& GetArray() const;
  const base::flat_map<std::string, std::unique_ptr<CBORValue>>& GetMap() const;

 protected:
  Type type_;

  union {
    uint64_t unsigned_value_;
    base::ManualConstructor<std::vector<uint8_t>> bytestring_value_;
    base::ManualConstructor<base::StringPiece> string_value_;
    base::ManualConstructor<std::vector<CBORValue>> array_value_;
    base::ManualConstructor<
        base::flat_map<std::string, std::unique_ptr<CBORValue>>>
        map_value_;
  };

  void InternalMoveConstructFrom(CBORValue&& that);
  void InternalCleanup();
  DISALLOW_COPY_AND_ASSIGN(CBORValue);
};

// MapValue wraps the construction of a map to be CBOR-encoded.
// Keys are |std::string|s and should be UTF-8 encoded.
class CONTENT_EXPORT MapValue : public CBORValue {
 public:
  MapValue();
  explicit MapValue(
      const base::flat_map<std::string, std::unique_ptr<CBORValue>>& in_map);

  ~MapValue();

  size_t size() const { return map_value_->size(); }

  void Insert(std::string key, base::StringPiece string);

  void Insert(std::string key, uint64_t value);

  void Insert(std::string key, std::vector<uint8_t>& bytestring);

  void Insert(std::string key, std::vector<CBORValue>& array);

  void Insert(std::string key,
              base::flat_map<std::string, std::unique_ptr<CBORValue>>& map);

  void Insert(std::string key, MapValue map);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_VALUES_H_
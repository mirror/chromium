// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VALUES_H_
#define VALUES_H_

#include <memory>
#include <vector>

#include "base/strings/string16.h"
#include "base/strings/string_piece.h"

namespace base {

class Value {
 public:
  enum class Type {
    NONE = 0,
    BOOLEAN,
    INTEGER,
    DOUBLE,
    STRING,
    BINARY,
    DICTIONARY,
    LIST
  };

  // Returns the type of the value stored by the current Value object.
  Type GetType() const { return type_; }  // DEPRECATED, use type().
  Type type() const { return type_; }

 private:
  Type type_ = Type::NONE;
};

// FundamentalValue represents the simple fundamental types of values.
class FundamentalValue : public Value {
 public:
  explicit FundamentalValue(bool in_value);
  explicit FundamentalValue(int in_value);
  explicit FundamentalValue(double in_value);
};

class StringValue : public Value {
 public:
  // Initializes a StringValue with a UTF-8 narrow character string.
  explicit StringValue(StringPiece in_value);

  // Initializes a StringValue with a string16.
  explicit StringValue(const string16& in_value);
};

// Stub base::ListValue class that supports Append(Value*).
class ListValue : public Value {
 public:
  ListValue();

  // Appends a Value to the end of the list.
  void Append(std::unique_ptr<Value> in_value);

  // Deprecated version of the above.
  void Append(Value* in_value);

  // Convenience forms of Append.
  void AppendBoolean(bool in_value);
  void AppendInteger(int in_value);
  void AppendDouble(double in_value);
  void AppendString(StringPiece in_value);
  void AppendString(const string16& in_value);
  void AppendStrings(const std::vector<std::string>& in_values);
  void AppendStrings(const std::vector<string16>& in_values);
};

}  // namespace base

#endif  // VALUES_H_

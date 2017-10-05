// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "values.h"

namespace {

class Preference {
 public:
  Preference() : type_(base::Value::Type::NONE) {}
  base::Value::Type GetType() const { return type_; }

 private:
  const base::Value::Type type_;
};

}  // namespace

bool F() {
  base::Value value;
  return value.GetType() == base::Value::Type::BINARY;
}

bool G() {
  base::Value value_1, value_2;
  return value_1.GetType() == value_2.GetType();
}

bool H() {
  Preference pref;
  return pref.GetType() == base::Value::Type::DICTIONARY;
}

bool I() {
  Preference pref_1, pref_2;
  return pref_1.GetType() == pref_2.GetType();
}

bool J() {
  Preference pref;
  base::Value val;
  return pref.GetType() == val.GetType();
}

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSNumericValueType_h
#define CSSNumericValueType_h

#include "core/css/CSSPrimitiveValue.h"

#include <array>

namespace blink {

// Represents the type of a CSSNumericValue, which is a map of base types to
// integers, and an associated percent hint.
// https://drafts.css-houdini.org/css-typed-om-1/#numeric-typing
class CSSNumericValueType {
 public:
  enum class BaseType : unsigned {
    kLength,
    kAngle,
    kTime,
    kFrequency,
    kResolution,
    kFlex,
    kPercent,
    kNumBaseTypes
  };

  static constexpr unsigned kNumBaseTypes =
      static_cast<unsigned>(BaseType::kNumBaseTypes);

  explicit CSSNumericValueType(CSSPrimitiveValue::UnitType);

  static CSSNumericValueType NegateEntries(CSSNumericValueType);

  int& Entry(BaseType type) {
    DCHECK_LT(type, BaseType::kNumBaseTypes);
    return entries_[static_cast<unsigned>(type)];
  }

  int Entry(BaseType type) const {
    DCHECK_LT(type, BaseType::kNumBaseTypes);
    return entries_[static_cast<unsigned>(type)];
  }

  bool HasPercentHint() const {
    // TODO(776173): Implement this
    return false;
  }

 private:
  std::array<int, kNumBaseTypes> entries_{};  // zero-initialize
};

}  // namespace blink

#endif  // CSSNumericValueType_h

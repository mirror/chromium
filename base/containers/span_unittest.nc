// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a "No Compile Test" suite.
// http://dev.chromium.org/developers/testing/no-compile-tests

#include "base/containers/span.h"

#include <array>

namespace base {

class Parent {
};

class Child : Parent {
};

#if defined(NCTEST_DERIVED_TO_BASE_CONVERSION_DISALLOWED)  // [r"fatal error: no matching constructor for initialization of 'Span<base::Parent \*>'"]

// Internally, this is represented as a pointer to pointers to Derived. An
// implicit conversion to a pointer to pointers to Base must not be allowed.
// If it were allowed, then something like this would be possible.
//   Cat** cats = GetCats();
//   Animals** animals = cats;
//   animals[0] = new Dog();  // Uhoh!
void WontCompile() {
  Span<Child*> child_span;
  Span<Parent*> parent_span(child_span);
}

#elif defined(NCTEST_PTR_TO_CONSTPTR_CONVERSION_DISALLOWED)  // [r"fatal error: no matching constructor for initialization of 'Span<const int \*>'"]

// Similarly, converting a Span<int*> to Span<const int*> requires internally
// converting T** to const T**. This is also disallowed, as it would allow code
// to violate the contract of const.
void WontCompile() {
  Span<int*> non_const_span;
  Span<const int*> const_span(non_const_span);
}

#elif defined(NCTEST_STD_ARRAY_CONVERSION_DISALLOWED)  // [r"fatal error: no matching constructor for initialization of 'Span<int>'"]

// This isn't implemented today. Maybe it will be some day.
void WontCompile() {
  std::array<int, 3> arr;
  Span<int> span(arr);
}

#endif

}  // namespace base

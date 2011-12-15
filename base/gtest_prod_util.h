// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_GTEST_PROD_UTIL_H_
#define BASE_GTEST_PROD_UTIL_H_
#pragma once

#undef FRIEND_TEST

#include "testing/gtest/include/gtest/gtest_prod.h"


#if defined(GOOGLE_CHROME_BUILD)

#undef FRIEND_TEST

// Provide a no-op that can live in a class definition.
// We can't use an expression, so we define a useless local class name.
#define FRIEND_TEST(test_case_name, test_name)                                 \
  class test_case_name##_##test_name##_Test {int garbage;}

#endif



// This is a wrapper for gtest's FRIEND_TEST macro that friends
// test with all possible prefixes. This is very helpful when changing the test
// prefix, because the friend declarations don't need to be updated.
//
// Example usage:
//
// class MyClass {
//  private:
//   void MyMethod();
//   FRIEND_TEST_ALL_PREFIXES(MyClassTest, MyMethod);
// };
#define FRIEND_TEST_ALL_PREFIXES(test_case_name, test_name) \
  FRIEND_TEST(test_case_name, test_name); \
  FRIEND_TEST(test_case_name, DISABLED_##test_name); \
  FRIEND_TEST(test_case_name, FLAKY_##test_name); \
  FRIEND_TEST(test_case_name, FAILS_##test_name)

#endif  // BASE_GTEST_PROD_UTIL_H_

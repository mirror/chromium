// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/macros.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

class SimpleTest : public testing::Test {
 public:
  SimpleTest() = default;
  ~SimpleTest() override = default;

 protected:
  void ExpectLhsLessThanOrEqualRhs(int lhs, int rhs) { EXPECT_LE(lhs, rhs); }

  void ExpectSorted(int n1, int n2, int n3) {
    SCOPED_TRACE(std::string("ExpectSorted: ") + std::to_string(n1) + ", " +
                 std::to_string(n2) + ", " + std::to_string(n3));

    {
      SCOPED_TRACE("Test first two numbers");
      int lhs = n1;
      int rhs = n2;
      ExpectLhsLessThanOrEqualRhs(lhs, rhs);
    }

    {
      SCOPED_TRACE("Test second two numbers");
      int lhs = n2;
      int rhs = n3;
      ExpectLhsLessThanOrEqualRhs(lhs, rhs);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SimpleTest);
};

TEST_F(SimpleTest, Sort) {
  SCOPED_TRACE("Trace: SimpleTest.Sort test");
  int n1 = 1;
  int n2 = 42;
  int n3 = 9000;

  {
    SCOPED_TRACE("Trace: SimpleTest.Sort ExpectSorted call 1");
    ExpectSorted(n1, n2, n3);
  }

  {
    SCOPED_TRACE("Trace: SimpleTest.Sort ExpectSorted call 2");
    ExpectSorted(n2, n3, n1);
  }
}

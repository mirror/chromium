// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/can_make_payment_query.h"

#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace payments {

struct TestCase {
  TestCase(const char* first_origin,
           const char* second_origin,
           bool expected_second_origin_can_make_payment)
      : first_origin(first_origin),
        second_origin(second_origin),
        expected_second_origin_can_make_payment(
            expected_second_origin_can_make_payment) {}

  ~TestCase() {}

  const char* const first_origin;
  const char* const second_origin;
  const bool expected_second_origin_can_make_payment;
};

class CanMakePaymentQueryTest : public testing::TestWithParam<TestCase> {};

TEST_P(CanMakePaymentQueryTest, Test) {
  std::map<std::string, std::set<std::string>> query;
  CanMakePaymentQuery can_make_payment_query;
  EXPECT_TRUE(can_make_payment_query.CanQuery(
      GURL(GetParam().first_origin), GURL(GetParam().first_origin), query));

  EXPECT_EQ(
      GetParam().expected_second_origin_can_make_payment,
      can_make_payment_query.CanQuery(GURL(GetParam().second_origin),
                                      GURL(GetParam().second_origin), query));
}

INSTANTIATE_TEST_CASE_P(
    Allowed,
    CanMakePaymentQueryTest,
    testing::Values(
        TestCase("https://example.com", "https://example.com", true),
        TestCase("http://localhost", "http://localhost", true),
        TestCase("file:///tmp/test.html", "file:///tmp/test.html", true)));

INSTANTIATE_TEST_CASE_P(
    Denied,
    CanMakePaymentQueryTest,
    testing::Values(
        TestCase("https://example.com", "https://not-example.com", false),
        TestCase("http://localhost", "http://not-localhost", false),
        TestCase("file:///tmp/test.html", "file:///tmp/not-test.html", false)));

}  // namespace payments

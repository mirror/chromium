// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// #include "base/command_line.h"
// #include "base/files/file_path.h"
// #include "base/files/file_util.h"
// #include "base/process/launch.h"
// #include "base/strings/utf_string_conversions.h"
#include "tools/traffic_annotation/auditor/traffic_annotation_auditor.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "testing/gtest/include/gtest/gtest.h"

using namespace testing;

class TrafficAnnotationAuditorTest : public ::testing::Test {
 public:
  // void SetUp() override {}

 protected:
  // base::FilePath hash_value_script_;
};

// Tests if the two hash computation functions have the same result.
TEST_F(TrafficAnnotationAuditorTest, HashFunctionCheck) {
#define TEST_HASH_CODE(X)                                    \
  EXPECT_EQ(traffic_annotation_auditor::ComputeHashValue(X), \
            net::DefineNetworkTrafficAnnotation(X, "").unique_id_hash_code);

  TEST_HASH_CODE("test")
  TEST_HASH_CODE("unique_id")
  TEST_HASH_CODE("123_id")
  TEST_HASH_CODE("ID123")
  TEST_HASH_CODE("a_unique_looooooooooooooooooooooooooooooooooooooooooooong_id")
#undef TEST_HASH_CODE
}

// // Just written to ensure this is running.
// TEST_F(TrafficAnnotationAuditorTest, DesignedToFailTest) {
//   EXPECT_EQ(traffic_annotation_auditor::ComputeHashValue("Fail Me"), 747);
// }

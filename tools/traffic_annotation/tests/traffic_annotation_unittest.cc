// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/process/launch.h"
#include "base/strings/utf_string_conversions.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "testing/gtest/include/gtest/gtest.h"

using namespace testing;

namespace network_traffic_annotations {

class NetworkTrafficAnnotationsTest : public ::testing::Test {
 public:
  void SetUp() override {
    hash_value_script_ =
        base::FilePath(FILE_PATH_LITERAL("tools"))
            .Append(base::FilePath(FILE_PATH_LITERAL("traffic_annotation"))
                        .Append(base::FilePath(FILE_PATH_LITERAL("auditor"))
                                    .Append(base::FilePath(FILE_PATH_LITERAL(
                                        "compute_hash_value.py")))));
  }

  // Runs a python script and returns the result.
  int RunPythonScript(const base::FilePath& script_path,
                      const std::string& arguments) {
    base::CommandLine cmdline(script_path);
#ifdef _WIN32
    cmdline.AppendArgNative(base::ASCIIToUTF16(arguments));
#else
    cmdline.AppendArgNative(arguments);
#endif

    std::string output;
    if (base::GetAppOutput(cmdline, &output)) {
#ifndef _WIN32
      if (!output.length())
        LOG(ERROR) << cmdline.GetArgumentsString().c_str();
#endif
      return atoi(output.c_str());
    } else {
      base::FilePath fp;
      base::GetCurrentDirectory(&fp);
      LOG(ERROR) << "Script call failed. " << fp.value().c_str() << " "
                 << cmdline.GetArgumentsString().c_str();
      return -1;
    }
  }

 protected:
  base::FilePath hash_value_script_;
};

// Tests if the python and C++ hash computation functions have the same result.
TEST_F(NetworkTrafficAnnotationsTest, HashFunctionCheck) {
#define TEST_HASH_CODE(X)                           \
  EXPECT_EQ(RunPythonScript(hash_value_script_, X), \
            net::DefineNetworkTrafficAnnotation(X, "").unique_id_hash_code);

  TEST_HASH_CODE("test")
  TEST_HASH_CODE("unique_id")
  TEST_HASH_CODE("a_unique_looooooooooooooooooooooooooooooooooooooooooooong_id")
#undef TEST_HASH_CODE
}

// Just written to ensure this is running.
TEST_F(NetworkTrafficAnnotationsTest, DesignedToFailTest) {
  EXPECT_EQ(RunPythonScript(hash_value_script_, "Fail Me"), 747);
}

}  // namespace battor

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/process/launch.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "testing/gtest/include/gtest/gtest.h"

using namespace testing;

namespace network_traffic_annotations {

// Runs a python script and returns the result.
int RunPythonScript(const base::FilePath& script_path,
                    const std::string& arguments) {
  base::CommandLine cmdline(script_path);
  cmdline.AppendArgNative(arguments);

  int exit_code = -1;
  std::string output;
  if (base::GetAppOutputWithExitCode(cmdline, &output, &exit_code)) {
    return atoi(output.c_str());
  } else
    return -1;
}

TEST(NetworkTrafficAnnotationsTest, HashFunctionCheck) {
  const base::FilePath hash_value_script =
      base::FilePath(FILE_PATH_LITERAL("tools"))
          .Append(base::FilePath(FILE_PATH_LITERAL("traffic_annotation"))
                      .Append(base::FilePath(FILE_PATH_LITERAL("auditor"))
                                  .Append(base::FilePath(FILE_PATH_LITERAL(
                                      "compute_hash_value.py")))));

#define TEST_HASH_CODE(X)                          \
  EXPECT_EQ(RunPythonScript(hash_value_script, X), \
            net::DefineNetworkTrafficAnnotation(X, "").unique_id_hash_code);

  TEST_HASH_CODE("test")
  TEST_HASH_CODE("unique_id")
  TEST_HASH_CODE("an_absoulooooooooooooooooooooooouly_looooooooooooooooong_one")

#undef TEST_HASH_CODE
}

}  // namespace battor
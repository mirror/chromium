// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/optimization_guide/optimization_hints_reader.h"

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/version.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace optimization_guide {

class OptimizationHintsReaderTest : public testing::Test {
 public:
  OptimizationHintsReaderTest() {}
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    hints_reader_.reset(new OptimizationHintsReader());
  }

  OptimizationHintsReader* optimization_hints_reader() {
    return hints_reader_.get();
  }

  base::FilePath GetHintsDir() { return temp_dir_.GetPath(); }

 protected:
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<OptimizationHintsReader> hints_reader_;

 private:
  DISALLOW_COPY_AND_ASSIGN(OptimizationHintsReaderTest);
};

TEST_F(OptimizationHintsReaderTest, NoHintsVersion) {
  const base::FilePath hints_root_dir(GetHintsDir());
  const base::FilePath hints_file =
      hints_root_dir.Append(FILE_PATH_LITERAL("optimization-hints.pb"));
  base::WriteFile(hints_file, "foo", 3);

  UnindexedHintsInfo unindexed_hints_info(base::Version(), hints_file);
  ASSERT_FALSE(optimization_hints_reader()->ReadHints(unindexed_hints_info));
}

TEST_F(OptimizationHintsReaderTest, InvalidHintsPath) {
  const base::FilePath hints_root_dir(GetHintsDir());
  const base::FilePath garbage_file =
      hints_root_dir.Append(FILE_PATH_LITERAL("somegarbagefile"));

  UnindexedHintsInfo unindexed_hints_info(base::Version("1.2.3"), garbage_file);
  ASSERT_FALSE(optimization_hints_reader()->ReadHints(unindexed_hints_info));
}

TEST_F(OptimizationHintsReaderTest, ValidUnindexedHintsInfo) {
  const base::FilePath hints_root_dir(GetHintsDir());
  const base::FilePath hints_file =
      hints_root_dir.Append(FILE_PATH_LITERAL("optimization-hints.pb"));
  base::WriteFile(hints_file, "blah", 4);

  UnindexedHintsInfo unindexed_hints_info(base::Version("1.2.3"), hints_file);
  ASSERT_TRUE(optimization_hints_reader()->ReadHints(unindexed_hints_info));
}

}  // namespace optimization_guide

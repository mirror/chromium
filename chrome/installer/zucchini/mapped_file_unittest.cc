// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/mapped_file.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

class MappedFileWriterTest : public testing::Test {
 public:
  MappedFileWriterTest() {}
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    file_name_ = temp_dir_.GetPath().AppendASCII("test-file");
  }

  base::FilePath file_name_;

 private:
  base::ScopedTempDir temp_dir_;
};

TEST_F(MappedFileWriterTest, DeleteOnClose) {
  EXPECT_FALSE(base::PathExists(file_name_));
  { MappedFileWriter file_writer(file_name_, 10); }
  EXPECT_FALSE(base::PathExists(file_name_));
}

TEST_F(MappedFileWriterTest, Keep) {
  EXPECT_FALSE(base::PathExists(file_name_));
  {
    MappedFileWriter file_writer(file_name_, 10);
    EXPECT_TRUE(file_writer.Keep());
  }
  EXPECT_TRUE(base::PathExists(file_name_));
}

}  // namespace zucchini

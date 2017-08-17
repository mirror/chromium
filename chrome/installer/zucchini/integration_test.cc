// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "base/files/file_path.h"
#include "base/files/memory_mapped_file.h"
#include "base/path_service.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/patch_reader.h"
#include "chrome/installer/zucchini/patch_writer.h"
#include "chrome/installer/zucchini/zucchini.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

base::FilePath MakeTestPath(const std::string& filename) {
  base::FilePath path;
  EXPECT_TRUE(PathService::Get(base::DIR_SOURCE_ROOT, &path));
  return path.AppendASCII("chrome")
      .AppendASCII("installer")
      .AppendASCII("zucchini")
      .AppendASCII("testdata")
      .AppendASCII(filename);
}

void TestGenApply(const std::string& old_filename,
                  const std::string& new_filename,
                  bool raw) {
  base::FilePath old_path = MakeTestPath(old_filename);
  base::FilePath new_path = MakeTestPath(new_filename);

  base::MemoryMappedFile old_file;
  ASSERT_TRUE(old_file.Initialize(old_path));

  base::MemoryMappedFile new_file;
  ASSERT_TRUE(new_file.Initialize(new_path));

  ConstBufferView old_region(old_file.data(), old_file.length());
  ConstBufferView new_region(new_file.data(), new_file.length());

  EnsemblePatchWriter patch_writer(old_region, new_region);

  auto generate = raw ? GenerateRaw : GenerateEnsemble;
  ASSERT_EQ(status::kStatusSuccess,
            generate(old_region, new_region, &patch_writer));

  std::vector<uint8_t> patch_buffer(patch_writer.SerializedSize());
  patch_writer.SerializeInto({patch_buffer.data(), patch_buffer.size()});

  auto patch_reader =
      EnsemblePatchReader::Create({patch_buffer.data(), patch_buffer.size()});
  ASSERT_TRUE(patch_reader.has_value());

  ASSERT_EQ(new_file.length(), patch_reader->header().new_size);
  std::vector<uint8_t> new_generated_buffer(new_file.length());
  ASSERT_EQ(status::kStatusSuccess,
            Apply(old_region, *patch_reader,
                  {new_generated_buffer.data(), new_generated_buffer.size()}));

  EXPECT_TRUE(std::equal(new_region.begin(), new_region.end(),
                         new_generated_buffer.begin()));
}

TEST(EndToEndTest, GenApply) {
  TestGenApply("setup1.exe", "setup2.exe", true);
  TestGenApply("setup1.exe", "setup2.exe", false);
  TestGenApply("chrome64_1.exe", "chrome64_2.exe", true);
  TestGenApply("chrome64_1.exe", "chrome64_2.exe", false);
}

}  // namespace zucchini

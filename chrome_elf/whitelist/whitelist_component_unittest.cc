// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/whitelist/whitelist_component.h"

#include <windows.h>

#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/win/pe_image.h"
#include "chrome/install_static/user_data_dir.h"
#include "chrome_elf/sha1/sha1.h"
#include "chrome_elf/whitelist/whitelist_component_format.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace whitelist {
namespace {

constexpr wchar_t kComponentFilename[] = L"dbfile";

// Test binaries in system32.
// Define them in hash order so that the resulting array is ordered!
// hal = 0b bc 39..., gdi32 = 91 7a e5..., crypt32 = ce ab 70...
const wchar_t* const kComponentTestBins[] = {L"hal.dll", L"gdi32.dll",
                                             L"gdi32.dll", L"crypt32.dll"};
const char* const kComponentTestBinBasenames[] = {"hal.dll", "gdi32.dll",
                                                  "gdi32.dll", "crypt32.dll"};

constexpr DWORD kPageSize = 4096;
constexpr uint32_t kTestArraySize = arraysize(kComponentTestBins);
static_assert(kTestArraySize == arraysize(kComponentTestBinBasenames),
              "Whitelist component test arrays should be the same size!");

struct TestModule {
  std::string basename;
  DWORD timedatestamp;
  DWORD imagesize;
};

bool GetTestModules(std::vector<TestModule>* test_modules,
                    std::vector<PackedWhitelistModule>* packed_modules) {
  // Get test data from system binaries.
  base::FilePath path;
  char buffer[kPageSize];
  PIMAGE_NT_HEADERS nt_headers = nullptr;
  std::string code_id;
  std::string basename_hash;
  test_modules->clear();
  packed_modules->clear();

  for (size_t i = 0; i < kTestArraySize; i++) {
    if (!base::PathService::Get(base::DIR_SYSTEM, &path))
      return false;
    path = path.Append(kComponentTestBins[i]);
    base::File binary(path, base::File::FLAG_READ | base::File::FLAG_OPEN);
    if (!binary.IsValid())
      return false;
    if (binary.Read(0, &buffer[0], kPageSize) != kPageSize)
      return false;
    base::win::PEImage pe_image(buffer);
    if (!pe_image.VerifyMagic())
      return false;
    nt_headers = pe_image.GetNTHeaders();

    // Save the module info for tests.
    TestModule test_module;
    test_module.basename.assign(kComponentTestBinBasenames[i]);
    test_module.timedatestamp = nt_headers->FileHeader.TimeDateStamp;
    test_module.imagesize = nt_headers->OptionalHeader.SizeOfImage;
    test_modules->push_back(test_module);

    // SHA1 hash the two strings, and copy them into the module array.
    code_id = base::StringPrintf("%08lX%lx", test_module.timedatestamp,
                                 test_module.imagesize);
    code_id = elf_sha1::SHA1HashString(code_id);
    basename_hash = elf_sha1::SHA1HashString(test_module.basename);

    PackedWhitelistModule packed_module;
    ::memcpy(packed_module.code_id_hash, code_id.data(), elf_sha1::kSHA1Length);
    ::memcpy(packed_module.basename_hash, basename_hash.data(),
             elf_sha1::kSHA1Length);
    packed_modules->push_back(packed_module);
  }

  return true;
}

//------------------------------------------------------------------------------
// WhitelistComponentTest class
//------------------------------------------------------------------------------

class WhitelistComponentTest : public testing::Test {
 protected:
  WhitelistComponentTest() {}

  void SetUp() override {
    std::vector<PackedWhitelistModule> test_file_array;
    ASSERT_TRUE(GetTestModules(&test_array_, &test_file_array));

    // Setup test component file.
    ASSERT_TRUE(scoped_temp_dir_.CreateUniqueTempDir());
    base::FilePath path = scoped_temp_dir_.GetPath();

    // Create the component file.  It will be cleaned up with
    // |scoped_temp_dir_|.
    path = path.Append(kComponentFilename);
    base::File file(path, base::File::FLAG_CREATE_ALWAYS |
                              base::File::FLAG_WRITE |
                              base::File::FLAG_SHARE_DELETE);
    ASSERT_TRUE(file.IsValid());

    // Store the path for tests to use.
    test_file_path_ = std::move(path.value());

    // Write content {metadata}{array_of_modules}.
    PackedWhitelistMetadata meta = {kInitialVersion, kTestArraySize};
    ASSERT_TRUE(file.Write(0, reinterpret_cast<const char*>(&meta),
                           sizeof(meta)) == sizeof(meta));
    int size = test_file_array.size() * sizeof(PackedWhitelistModule);
    ASSERT_TRUE(
        file.Write(sizeof(PackedWhitelistMetadata),
                   reinterpret_cast<const char*>(test_file_array.data()),
                   size) == size);
  }

  const base::string16& GetTestFilePath() { return test_file_path_; }

  const std::vector<TestModule>& GetTestArray() { return test_array_; }

 private:
  base::ScopedTempDir scoped_temp_dir_;
  base::string16 test_file_path_;
  std::vector<TestModule> test_array_;

  DISALLOW_COPY_AND_ASSIGN(WhitelistComponentTest);
};

//------------------------------------------------------------------------------
// Whitelist component tests
//------------------------------------------------------------------------------

// Test initialization of the whitelist component.
TEST_F(WhitelistComponentTest, ComponentInit) {
  // Override the component file path for testing.
  OverrideComponentFilePathForTesting(GetTestFilePath());

  // Init component whitelist!
  ASSERT_EQ(InitComponent(), ComponentStatus::kSuccess);

  // Test matching.
  for (size_t i = 0; i < kTestArraySize; i++) {
    TestModule temp = GetTestArray().at(i);
    ASSERT_TRUE(
        IsModuleWhitelisted(temp.basename, temp.imagesize, temp.timedatestamp));
  }

  // Test a failure to match.
  ASSERT_FALSE(IsModuleWhitelisted(std::string("booya.dll"), 1337, 0x12345678));
}

}  // namespace
}  // namespace whitelist

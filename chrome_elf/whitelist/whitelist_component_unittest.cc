// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/whitelist/whitelist_component.h"

#include <windows.h>

#include <string>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/win/pe_image.h"
#include "chrome/install_static/user_data_dir.h"
#include "chrome_elf/sha1/sha1.h"
#include "chrome_elf/whitelist/whitelist_component_format.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace whitelist {
namespace {

constexpr wchar_t kComponentSubdir[] = L"\\ThirdPartyModuleList64";
constexpr wchar_t kComponentFilename[] = L"\\dbfile.txt";

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

bool GetTestModules(TestModule* test_modules,
                    PackedWhitelistModule* packed_modules) {
  // Get test data from system binaries.
  base::FilePath path;
  char buffer[kPageSize];
  PIMAGE_NT_HEADERS nt_headers = nullptr;
  std::string code_id;
  std::string basename_hash;

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
    test_modules[i].basename.assign(kComponentTestBinBasenames[i]);
    test_modules[i].timedatestamp = nt_headers->FileHeader.TimeDateStamp;
    test_modules[i].imagesize = nt_headers->OptionalHeader.SizeOfImage;

    // SHA1 hash the two strings, and copy them into the module array.
    code_id = base::StringPrintf("%08lX%lx", test_modules[i].timedatestamp,
                                 test_modules[i].imagesize);
    code_id = elf_sha1::SHA1HashString(code_id);
    basename_hash = elf_sha1::SHA1HashString(test_modules[i].basename);

    ::memcpy(packed_modules[i].code_id_hash, code_id.c_str(),
             elf_sha1::kSHA1Length);
    ::memcpy(packed_modules[i].basename_hash, basename_hash.c_str(),
             elf_sha1::kSHA1Length);
  }

  return true;
}

//------------------------------------------------------------------------------
// WhitelistComponentTest class
//------------------------------------------------------------------------------

class WhitelistComponentTest : public testing::Test {
 protected:
  // TODO(pennymac): Update this after crbug/772168.
  void SetUp() override {
    PackedWhitelistModule test_file_array[kTestArraySize] = {};
    ASSERT_TRUE(GetTestModules(test_array_, test_file_array));

    // Ensure path exists.
    base::string16 path;
    ASSERT_TRUE(install_static::GetUserDataDirectory(&path, nullptr));
    path.append(kComponentSubdir);
    ASSERT_TRUE(
        base::CreateDirectoryAndGetError(base::FilePath(path), nullptr));

    // Create the component file.
    path.append(kComponentFilename);
    base::File file(base::FilePath(path),
                    base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
    ASSERT_TRUE(file.IsValid());
    test_file_path_ = std::move(path);

    // Write content {metadata}{array_of_modules}.
    PackedWhitelistMetadata meta = {kInitialVersion, kTestArraySize};
    ASSERT_TRUE(file.Write(0, reinterpret_cast<const char*>(&meta),
                           sizeof(meta)) == sizeof(meta));
    ASSERT_TRUE(file.Write(sizeof(PackedWhitelistMetadata),
                           reinterpret_cast<const char*>(test_file_array),
                           sizeof(test_file_array)) == sizeof(test_file_array));
  }

  void TearDown() override {
    // Make sure any test files get cleaned up.
    base::DeleteFileW(base::FilePath(test_file_path_), false);
  }

  TestModule* GetTestArray() { return test_array_; }

 private:
  base::string16 test_file_path_;
  TestModule test_array_[kTestArraySize] = {};
};

//------------------------------------------------------------------------------
// Whitelist component tests
//------------------------------------------------------------------------------

// Test initialization of the whitelist component.
TEST_F(WhitelistComponentTest, ComponentInit) {
  // Init component whitelist!
  EXPECT_EQ(InitComponent(), ComponentStatus::kSuccess);

  // Test matching.
  for (size_t i = 0; i < kTestArraySize; i++) {
    TestModule temp = GetTestArray()[i];
    ASSERT_TRUE(
        WhitelistLookup(temp.basename, temp.imagesize, temp.timedatestamp));
  }

  // Test a failure to match.
  ASSERT_FALSE(WhitelistLookup(std::string("booya.dll"), 1337, 0x12345678));
}

}  // namespace
}  // namespace whitelist

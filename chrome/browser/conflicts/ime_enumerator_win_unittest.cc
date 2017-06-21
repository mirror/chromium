// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/ime_enumerator_win.h"

#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/test_reg_util_win.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Note: Because the purpose of ImeEnumerator is very similar to
//       ShellExtensionEnumerator, their tests are also quite alike.
class ImeEnumeratorTest : public testing::Test {
 public:
  ImeEnumeratorTest() = default;

  ~ImeEnumeratorTest() override = default;

  void SetUp() override {
    // Override all registry hives so that real IMEs don't mess up the unit
    // tests.
    ASSERT_NO_FATAL_FAILURE(
        registry_override_manager_.OverrideRegistry(HKEY_CLASSES_ROOT));
    ASSERT_NO_FATAL_FAILURE(
        registry_override_manager_.OverrideRegistry(HKEY_CURRENT_USER));
    ASSERT_NO_FATAL_FAILURE(
        registry_override_manager_.OverrideRegistry(HKEY_LOCAL_MACHINE));
  }

  void RunUntilIdle() { scoped_task_environment_.RunUntilIdle(); }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  registry_util::RegistryOverrideManager registry_override_manager_;

  DISALLOW_COPY_AND_ASSIGN(ImeEnumeratorTest);
};

// Adds a fake IME entry to the registry that should be found by the
// ImeEnumerator. Returns true on success.
bool RegisterFakeIme(const wchar_t* guid, const wchar_t* path) {
  base::win::RegKey class_id(
      HKEY_CLASSES_ROOT,
      base::StringPrintf(ImeEnumerator::kClassIdRegistryKeyFormat, guid)
          .c_str(),
      KEY_WRITE);
  if (!class_id.Valid())
    return false;

  if (class_id.WriteValue(nullptr, path) != ERROR_SUCCESS)
    return false;

  base::win::RegKey registration(HKEY_LOCAL_MACHINE,
                                 ImeEnumerator::kImeRegistryKey, KEY_WRITE);
  return registration.CreateKey(guid, KEY_WRITE) == ERROR_SUCCESS;
}

void OnImeEnumerated(std::vector<base::FilePath>* imes,
                     const base::FilePath& ime_path,
                     uint32_t size_of_image,
                     uint32_t time_date_stamp) {
  imes->push_back(ime_path);
}

}  // namespace

// Registers a few fake IMEs then see if the enumerator finds them.
TEST_F(ImeEnumeratorTest, EnumerateImes) {
  // Use the current exe file as an arbitrary module that exists.
  base::FilePath file_exe;
  ASSERT_TRUE(base::PathService::Get(base::FILE_EXE, &file_exe));
  EXPECT_TRUE(RegisterFakeIme(L"{FAKE_GUID}", file_exe.value().c_str()));

  // Do the asynchronous enumeration.
  std::vector<base::FilePath> imes;
  ImeEnumerator ime_enumerator(
      base::Bind(&OnImeEnumerated, base::Unretained(&imes)));

  RunUntilIdle();

  EXPECT_EQ(1u, imes.size());
  EXPECT_EQ(file_exe, imes[0]);
}

TEST_F(ImeEnumeratorTest, SkipMicrosoftImes) {
  const wchar_t kMicrosoftImeExample[] =
      L"{6a498709-e00b-4c45-a018-8f9e4081ae40}";

  // Register a fake IME using the Microsoft IME guid.
  EXPECT_TRUE(RegisterFakeIme(kMicrosoftImeExample, L"c:\\path\\to\\ime.dll"));

  // Do the asynchronous enumeration.
  std::vector<base::FilePath> imes;
  ImeEnumerator ime_enumerator(
      base::Bind(&OnImeEnumerated, base::Unretained(&imes)));

  RunUntilIdle();

  EXPECT_TRUE(imes.empty());
}

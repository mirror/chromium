// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/com_base_util.h"

#include "base/test/gtest_util.h"
#include "base/win/windows_version.h"

namespace base {
namespace win {

class ComBaseUtilTest : public testing::Test {
 public:
  ComBaseUtilTest() : expect_success_(false) {
    if (base::win::GetVersion() >= base::win::VERSION_WIN10_RS1)
      expect_success_ = true;
  }

  bool expectation() { return expect_success_; }

 private:
  bool expect_success_;
};

TEST_F(ComBaseUtilTest, PreloadFunctionInvalidData) {
  std::vector<std::string> functions;
  functions.push_back("InvalidFunctionName");
  EXPECT_FALSE(ComBaseUtil::PreloadFunctions(functions));

  // This test does succeed in loading any valid functions, so the dll should
  // not have loaded.
  EXPECT_EQ(nullptr, ComBaseUtil::GetLibraryForTesting());
}

TEST_F(ComBaseUtilTest, PreloadFunction) {
  std::vector<std::string> functions;
  functions.push_back("RoGetActivationFactory");
  functions.push_back("RoActivateInstance");
  functions.push_back("WindowsCreateStringReference");
  functions.push_back("WindowsCreateString");
  functions.push_back("WindowsDeleteString");
  functions.push_back("WindowsGetStringRawBuffer");
  EXPECT_EQ(expectation(), ComBaseUtil::PreloadFunctions(functions));

  EXPECT_TRUE(FreeLibrary(ComBaseUtil::GetLibraryForTesting()));
}

TEST_F(ComBaseUtilTest, WindowsCreateStringReference_NoPreload) {
  base::string16 data = L"Foo";
  HSTRING hstr;
  HSTRING_HEADER header;

  HRESULT hr = ComBaseUtil::WindowsCreateStringReference(
      data.c_str(), static_cast<uint32_t>(wcslen(data.c_str())), &header,
      &hstr);
  if (!expectation()) {
    EXPECT_EQ(E_ACCESSDENIED, hr);
    return;
  }
  EXPECT_TRUE(SUCCEEDED(hr));

  const base::char16* buffer =
      ComBaseUtil::WindowsGetStringRawBuffer(hstr, nullptr);
  EXPECT_STREQ(L"Foo", buffer);

  EXPECT_TRUE(FreeLibrary(ComBaseUtil::GetLibraryForTesting()));
}

TEST_F(ComBaseUtilTest, WindowsCreateAndDeleteString_NoPreload) {
  base::string16 data = L"Foo";
  HSTRING hstr;

  HRESULT hr = ComBaseUtil::WindowsCreateString(
      data.c_str(), static_cast<uint32_t>(wcslen(data.c_str())), &hstr);
  if (!expectation()) {
    EXPECT_EQ(E_ACCESSDENIED, hr);
    return;
  }
  EXPECT_TRUE(SUCCEEDED(hr));

  const base::char16* buffer =
      ComBaseUtil::WindowsGetStringRawBuffer(hstr, nullptr);
  EXPECT_STREQ(L"Foo", buffer);

  hr = ComBaseUtil::WindowsDeleteString(hstr);
  EXPECT_TRUE(SUCCEEDED(hr));

  EXPECT_TRUE(FreeLibrary(ComBaseUtil::GetLibraryForTesting()));
}

}  // namespace win
}  // namespace base

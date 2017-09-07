// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/scoped_hstring.h"

#include "base/test/gtest_util.h"
#include "base/win/com_base_util.h"
#include "base/win/windows_version.h"

namespace base {
namespace win {

namespace {

static const wchar_t kTestString1[] = L"123";
static const wchar_t kTestString2[] = L"456789";

}  // namespace

class ScopedHStringTest : public testing::Test {
 public:
  ScopedHStringTest() : expect_success_(false) {
    if (base::win::GetVersion() >= base::win::VERSION_WIN10_RS1)
      expect_success_ = true;
  }

  bool expectation() { return expect_success_; }

 private:
  bool expect_success_;
};

TEST_F(ScopedHStringTest, Init) {
  // ScopedHString requires ComBase, which is not available in older versions.
  if (base::win::GetVersion() < base::win::VERSION_WIN10_RS1)
    return;

  ScopedHString hstring(kTestString1);
  const base::char16* buffer =
      ScopedHString::WindowsGetStringRawBuffer(hstring.get(), nullptr);
  EXPECT_STREQ(kTestString1, buffer);

  hstring.reset();
  EXPECT_TRUE(hstring == NULL);
  EXPECT_EQ(NULL, hstring.get());

  ScopedHString hstring2(kTestString2);
  hstring.swap(hstring2);
  EXPECT_TRUE(hstring2 == NULL);

  buffer = ScopedHString::WindowsGetStringRawBuffer(hstring.get(), nullptr);
  EXPECT_STREQ(kTestString2, buffer);

  EXPECT_TRUE(FreeLibrary(ComBaseUtil::GetComBaseModuleHandle()));
}

TEST_F(ScopedHStringTest, WindowsStringApis_NoPreload) {
  // ScopedHString requires ComBase, which is not available in older versions.
  if (base::win::GetVersion() < base::win::VERSION_WIN10_RS1)
    return;

  base::string16 data = L"Foo";
  HSTRING hstr;

  HRESULT hr = base::win::ScopedHString::WindowsCreateString(
      data.c_str(), static_cast<uint32_t>(wcslen(data.c_str())), &hstr);
  if (!expectation()) {
    EXPECT_EQ(E_ACCESSDENIED, hr);
    return;
  }
  EXPECT_TRUE(SUCCEEDED(hr));

  const base::char16* buffer =
      base::win::ScopedHString::WindowsGetStringRawBuffer(hstr, nullptr);
  EXPECT_STREQ(L"Foo", buffer);

  hr = ScopedHString::WindowsDeleteString(hstr);
  EXPECT_TRUE(SUCCEEDED(hr));

  EXPECT_TRUE(FreeLibrary(ComBaseUtil::GetComBaseModuleHandle()));
}

}  // namespace win
}  // namespace base

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/scoped_hstring.h"

#include <winstring.h>

#include "base/strings/utf_string_conversions.h"
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

  ScopedHString hstring = ScopedHString::Create(kTestString1);
  std::string buffer = ScopedHString::HStringToString(hstring.get());
  EXPECT_STREQ(kTestString1, base::UTF8ToWide(buffer).c_str());

  hstring.reset();
  EXPECT_TRUE(hstring == NULL);
  EXPECT_EQ(NULL, hstring.get());

  ScopedHString hstring2 = ScopedHString::Create(kTestString2);
  hstring.swap(hstring2);
  EXPECT_TRUE(hstring2 == NULL);

  buffer = ScopedHString::HStringToString(hstring.get());
  EXPECT_STREQ(kTestString2, base::UTF8ToWide(buffer).c_str());

  EXPECT_TRUE(FreeLibrary(ComBaseUtil::GetComBaseModuleHandle()));
}

}  // namespace win
}  // namespace base

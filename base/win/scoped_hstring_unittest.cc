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

TEST(ScopedHStringTest, Init) {
  // ScopedHString requires ComBase, which is not available in older versions.
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;

  ScopedHString hstring = ScopedHString::Create(kTestString1);
  std::string buffer = hstring.GetString();
  EXPECT_STREQ(kTestString1, base::UTF8ToWide(buffer).c_str());
  base::StringPiece16 contents = hstring.GetString16();
  EXPECT_STREQ(kTestString1, contents.data());

  hstring.reset();
  EXPECT_TRUE(hstring == NULL);
  EXPECT_EQ(NULL, hstring.get());

  ScopedHString hstring2 = ScopedHString::Create(kTestString2);
  hstring.swap(hstring2);
  EXPECT_TRUE(hstring2 == NULL);

  buffer = hstring.GetString();
  EXPECT_STREQ(kTestString2, base::UTF8ToWide(buffer).c_str());
  contents = hstring.GetString16();
  EXPECT_STREQ(kTestString2, contents.data());
}

}  // namespace win
}  // namespace base

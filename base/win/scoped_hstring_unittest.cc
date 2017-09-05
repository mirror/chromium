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
size_t test1_len = arraysize(kTestString1) - 1;
size_t test2_len = arraysize(kTestString2) - 1;

}  // namespace

TEST(ScopedHStringTest, Init) {
  // ScopedHString requires ComBase, which is not available in older versions.
  if (base::win::GetVersion() < base::win::VERSION_WIN10_RS1)
    return;

  ScopedHString hstring(kTestString1);
  const base::char16* buffer =
      ComBaseUtil::WindowsGetStringRawBuffer(hstring.get(), nullptr);
  EXPECT_STREQ(kTestString1, buffer);

  hstring.reset();
  EXPECT_TRUE(hstring == NULL);
  EXPECT_EQ(NULL, hstring.get());

  ScopedHString hstring2(kTestString2);
  hstring.swap(hstring2);
  EXPECT_TRUE(hstring2 == NULL);

  buffer = ComBaseUtil::WindowsGetStringRawBuffer(hstring.get(), nullptr);
  EXPECT_STREQ(kTestString2, buffer);
}

}  // namespace win
}  // namespace base

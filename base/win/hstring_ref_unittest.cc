// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/gtest_util.h"
#include "base/win/com_base_util.h"
#include "base/win/hstring_ref.h"
#include "base/win/windows_version.h"

namespace base {
namespace win {

namespace {

static const wchar_t kTestString1[] = L"123";
size_t test1_len = arraysize(kTestString1) - 1;

} // namespace

TEST(HStringRefTest, Init) {
  // HStringRef requires ComBase, which is not available in older versions.
  if (base::win::GetVersion() < base::win::VERSION_WIN10_RS1)
    return;

  HStringRef hstring(kTestString1);
  const base::char16* buffer =
      ComBaseUtil::WindowsGetStringRawBuffer(hstring.Get(), nullptr);
  EXPECT_STREQ(kTestString1, buffer);
}

}  // namespace win
}  // namespace base

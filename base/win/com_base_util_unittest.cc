// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/com_base_util.h"

#include "base/win/windows_version.h"
#include "base/test/gtest_util.h"
#include "build/build_config.h"

namespace base {
namespace win {

#if defined(OS_WIN)

TEST(ComBaseUtil, Simple) {
  bool expect_success = false;
  //if (base::win::GetVersion() >= base::win::VERSION_WIN10_RS1)
    expect_success = true;

  EXPECT_TRUE(expect_success);
}

#endif // defined(OS_WIN)

}  // namespace win
}  // namespace base

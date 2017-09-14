// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/core_winrt_util.h"

#include "base/test/gtest_util.h"
#include "base/win/scoped_com_initializer.h"
#include "base/win/windows_version.h"

namespace base {
namespace win {

TEST(ComBaseUtilTest, PreloadFunctions) {
  ScopedCOMInitializer com_initializer(ScopedCOMInitializer::kMTA);

  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    EXPECT_FALSE(ResolveCoreWinRTDelayload());
  else
    EXPECT_TRUE(ResolveCoreWinRTDelayload());
}

}  // namespace win
}  // namespace base

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/com_base_util.h"

#include "base/test/gtest_util.h"
#include "base/win/scoped_com_initializer.h"
#include "base/win/windows_version.h"

namespace base {
namespace win {
namespace com_base_util {

TEST(ComBaseUtilTest, PreloadFunctions) {
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;

  ScopedCOMInitializer com_initializer(ScopedCOMInitializer::kMTA);
  EXPECT_TRUE(PreloadRequiredFunctions());
}

}  // namespace com_base_util
}  // namespace win
}  // namespace base

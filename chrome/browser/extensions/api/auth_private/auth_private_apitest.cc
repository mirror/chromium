// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "chrome/browser/extensions/api/auth_private/auth_private_api.h"
#include "chrome/browser/extensions/extension_apitest.h"

#if defined(OS_CHROMEOS)
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, AuthPrivate) {
  ASSERT_TRUE(RunComponentExtensionTest("auth_private")) << message_;
}
#endif

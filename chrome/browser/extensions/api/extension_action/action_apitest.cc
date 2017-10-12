// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace extensions {

class ActionApiTest : public ExtensionApiTest {};

IN_PROC_BROWSER_TEST_F(ActionApiTest, Basic) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(RunExtensionTest("action/basic")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;
}

}  // namespace extensions

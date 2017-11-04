// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"

namespace content {

class DummyBrowserTest : public ContentBrowserTest {
 public:
  DummyBrowserTest() {}

  void SetUp() override {
    LOG(ERROR) << "SetUp1";
    ContentBrowserTest::SetUp();
    LOG(ERROR) << "SetUp2";
  }

 private:
  // base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(DummyBrowserTest);
};

IN_PROC_BROWSER_TEST_F(DummyBrowserTest, Test3) {
  GURL url = GetTestUrl("", "simple_page.html");
  NavigateToURL(shell(), url);
}

}  // namespace content

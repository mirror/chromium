// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_lifecycle_unit_external.h"

#include "base/macros.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

namespace {

class DummyTabLifecycleUnitExternal : public TabLifecycleUnitExternal {
 public:
  explicit DummyTabLifecycleUnitExternal(content::WebContents* contents)
      : contents_(contents) {
    TabLifecycleUnitExternal::SetForWebContents(contents_, this);
  }

  ~DummyTabLifecycleUnitExternal() override {
    TabLifecycleUnitExternal::SetForWebContents(contents_, nullptr);
  }

  // TabLifecycleUnitExternal:
  content::WebContents* GetWebContents() const override { return nullptr; }
  bool IsAutoDiscardable() const override { return false; }
  void SetAutoDiscardable(bool) override {}
  void DiscardTab() override {}
  bool IsDiscarded() const override { return false; }

 private:
  content::WebContents* const contents_;

  DISALLOW_COPY_AND_ASSIGN(DummyTabLifecycleUnitExternal);
};

}  // namespace

using TabLifecycleUnitExternalTest = ChromeRenderViewHostTestHarness;

TEST_F(TabLifecycleUnitExternalTest, FromWebContentsNullByDefault) {
  EXPECT_FALSE(TabLifecycleUnitExternal::FromWebContents(web_contents()));
}

TEST_F(TabLifecycleUnitExternalTest, SetForWebContents) {
  {
    DummyTabLifecycleUnitExternal tab_lifecycle_unit_external(web_contents());
    EXPECT_EQ(&tab_lifecycle_unit_external,
              TabLifecycleUnitExternal::FromWebContents(web_contents()));
  }
  EXPECT_FALSE(TabLifecycleUnitExternal::FromWebContents(web_contents()));
  {
    DummyTabLifecycleUnitExternal tab_lifecycle_unit_external(web_contents());
    EXPECT_EQ(&tab_lifecycle_unit_external,
              TabLifecycleUnitExternal::FromWebContents(web_contents()));
  }
  EXPECT_FALSE(TabLifecycleUnitExternal::FromWebContents(web_contents()));
}

}  // namespace resource_coordinator

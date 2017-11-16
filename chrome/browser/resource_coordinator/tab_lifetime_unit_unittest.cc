// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_lifetime_unit.h"

#include "base/macros.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

namespace {

class DummyTabLifetimeUnit : public TabLifetimeUnit {
 public:
  explicit DummyTabLifetimeUnit(content::WebContents* contents)
      : contents_(contents) {
    TabLifetimeUnit::SetForWebContents(contents_, this);
  }

  ~DummyTabLifetimeUnit() override {
    TabLifetimeUnit::SetForWebContents(contents_, nullptr);
  }

  // TabLifetimeUnit:
  content::WebContents* GetWebContents() const override { return nullptr; }
  content::RenderProcessHost* GetRenderProcessHost() const override {
    return nullptr;
  }
  bool IsMediaTab() const override { return false; }
  bool IsAutoDiscardable() const override { return false; }
  void SetAutoDiscardable(bool) override {}

  // LifetimeUnit:
  base::string16 GetTitle() const override { return base::string16(); }
  SortKey GetSortKey() const override { return SortKey(); }
  State GetState() const override { return State::LOADED; }
  int GetEstimatedMemoryFreedOnDiscardKB() const override { return 0; }
  bool CanDiscard(DiscardCondition) const override { return false; }
  bool Discard(DiscardCondition) override { return false; }

 private:
  content::WebContents* const contents_;

  DISALLOW_COPY_AND_ASSIGN(DummyTabLifetimeUnit);
};

}  // namespace

using TabLifetimeUnitTest = ChromeRenderViewHostTestHarness;

TEST_F(TabLifetimeUnitTest, FromWebContentsNullByDefault) {
  EXPECT_FALSE(TabLifetimeUnit::FromWebContents(web_contents()));
}

TEST_F(TabLifetimeUnitTest, SetForWebContents) {
  {
    DummyTabLifetimeUnit tab_lifetime_unit(web_contents());
    EXPECT_EQ(&tab_lifetime_unit,
              TabLifetimeUnit::FromWebContents(web_contents()));
  }
  EXPECT_FALSE(TabLifetimeUnit::FromWebContents(web_contents()));
  {
    DummyTabLifetimeUnit tab_lifetime_unit(web_contents());
    EXPECT_EQ(&tab_lifetime_unit,
              TabLifetimeUnit::FromWebContents(web_contents()));
  }
  EXPECT_FALSE(TabLifetimeUnit::FromWebContents(web_contents()));
}

}  // namespace resource_coordinator

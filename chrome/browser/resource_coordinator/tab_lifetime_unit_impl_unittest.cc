// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_lifetime_unit_source.h"

#include <memory>
#include <vector>

#include "base/observer_list.h"
#include "base/test/simple_test_tick_clock.h"
#include "chrome/browser/resource_coordinator/tab_lifetime_observer.h"
#include "chrome/browser/resource_coordinator/tab_lifetime_unit_impl.h"
#include "chrome/browser/resource_coordinator/time.h"
#include "chrome/browser/ui/tabs/tab_strip_model_impl.h"
#include "chrome/browser/ui/tabs/test_tab_strip_model_delegate.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/navigation_controller.h"
#include "content/test/test_web_contents.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

namespace {

constexpr bool kUserGestureTrue = true;

class TabStripDummyDelegate : public TestTabStripModelDelegate {
 public:
  TabStripDummyDelegate() {}

  bool RunUnloadListenerBeforeClosing(content::WebContents* contents) override {
    return false;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TabStripDummyDelegate);
};

class TabLifetimeUnitSourceTest : public ChromeRenderViewHostTestHarness {
 protected:
  TabLifetimeUnitSourceTest()
      : scoped_set_tick_clock_for_testing_(&tick_clock_) {}

  content::WebContents* CreateWebContents() {
    content::TestWebContents* web_contents =
        content::TestWebContents::Create(profile(), nullptr);
    // Commit an URL to allow discarding.
    web_contents->NavigateAndCommit(GURL("https://www.example.com"));
    return web_contents;
  }

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    tab_strip_ = std::make_unique<TabStripModelImpl>(&tab_strip_model_delegate_,
                                                     profile());
    for (int i = 0; i < 3; ++i) {
      tab_strip_->AppendWebContents(CreateWebContents(), true);
      lifetime_units_.push_back(std::make_unique<TabLifetimeUnitImpl>(
          &observers_, tab_strip_->GetWebContentsAt(i), tab_strip_.get()));
    }
  }

  void TearDown() override {
    tab_strip_->CloseAllTabs();
    tab_strip_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

 private:
  TabStripDummyDelegate tab_strip_model_delegate_;

 protected:
  TabLifetimeUnitSource source_;
  base::SimpleTestTickClock tick_clock_;
  ScopedSetTickClockForTesting scoped_set_tick_clock_for_testing_;
  std::unique_ptr<TabStripModelImpl> tab_strip_;
  std::vector<std::unique_ptr<TabLifetimeUnitImpl>> lifetime_units_;
  base::ObserverList<TabLifetimeObserver> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TabLifetimeUnitSourceTest);
};

}  // namespace

TEST_F(TabLifetimeUnitSourceTest, SortKey) {
  lifetime_units_[0]->SetFocused(true);
  tick_clock_.Advance(base::TimeDelta::FromSeconds(1));
  lifetime_units_[0]->SetFocused(false);
  lifetime_units_[1]->SetFocused(true);
  tick_clock_.Advance(base::TimeDelta::FromSeconds(1));
  lifetime_units_[1]->SetFocused(false);
  lifetime_units_[2]->SetFocused(true);

  EXPECT_FALSE(lifetime_units_[0]->GetSortKey() <
               lifetime_units_[0]->GetSortKey());
  EXPECT_TRUE(lifetime_units_[0]->GetSortKey() <
              lifetime_units_[1]->GetSortKey());
  EXPECT_TRUE(lifetime_units_[0]->GetSortKey() <
              lifetime_units_[2]->GetSortKey());

  EXPECT_FALSE(lifetime_units_[1]->GetSortKey() <
               lifetime_units_[0]->GetSortKey());
  EXPECT_FALSE(lifetime_units_[1]->GetSortKey() <
               lifetime_units_[1]->GetSortKey());
  EXPECT_TRUE(lifetime_units_[1]->GetSortKey() <
              lifetime_units_[2]->GetSortKey());

  EXPECT_FALSE(lifetime_units_[2]->GetSortKey() <
               lifetime_units_[0]->GetSortKey());
  EXPECT_FALSE(lifetime_units_[2]->GetSortKey() <
               lifetime_units_[1]->GetSortKey());
  EXPECT_FALSE(lifetime_units_[2]->GetSortKey() <
               lifetime_units_[2]->GetSortKey());

  EXPECT_FALSE(lifetime_units_[0]->GetSortKey() >
               lifetime_units_[0]->GetSortKey());
  EXPECT_FALSE(lifetime_units_[0]->GetSortKey() >
               lifetime_units_[1]->GetSortKey());
  EXPECT_FALSE(lifetime_units_[0]->GetSortKey() >
               lifetime_units_[2]->GetSortKey());

  EXPECT_TRUE(lifetime_units_[1]->GetSortKey() >
              lifetime_units_[0]->GetSortKey());
  EXPECT_FALSE(lifetime_units_[1]->GetSortKey() >
               lifetime_units_[1]->GetSortKey());
  EXPECT_FALSE(lifetime_units_[1]->GetSortKey() >
               lifetime_units_[2]->GetSortKey());

  EXPECT_TRUE(lifetime_units_[2]->GetSortKey() >
              lifetime_units_[0]->GetSortKey());
  EXPECT_TRUE(lifetime_units_[2]->GetSortKey() >
              lifetime_units_[1]->GetSortKey());
  EXPECT_FALSE(lifetime_units_[2]->GetSortKey() >
               lifetime_units_[2]->GetSortKey());
}

TEST_F(TabLifetimeUnitSourceTest, Discard) {
  // Discard the first tab in the tab strip.
  EXPECT_NE(0, tab_strip_->active_index());
  EXPECT_EQ(LifetimeUnit::State::ALIVE, lifetime_units_[0]->GetState());
  auto* const initial_web_contents = tab_strip_->GetWebContentsAt(0);
  lifetime_units_[0]->Discard(DiscardCondition::NORMAL);
  EXPECT_EQ(LifetimeUnit::State::DISCARDED, lifetime_units_[0]->GetState());
  EXPECT_NE(initial_web_contents, tab_strip_->GetWebContentsAt(0));
  EXPECT_TRUE(
      tab_strip_->GetWebContentsAt(0)->GetController().IsInitialNavigation());

  // Activate the first tab in the tab strip. Expect it to be reloaded.
  tab_strip_->ActivateTabAt(0, kUserGestureTrue);
}

}  // namespace resource_coordinator

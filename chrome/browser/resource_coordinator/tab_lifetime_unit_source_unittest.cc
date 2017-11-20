// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_lifetime_unit_source.h"

#include <memory>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/observer_list.h"
#include "base/test/simple_test_tick_clock.h"
#include "build/build_config.h"
#include "chrome/browser/resource_coordinator/lifetime_unit_sink.h"
#include "chrome/browser/resource_coordinator/tab_lifetime_observer.h"
#include "chrome/browser/resource_coordinator/tab_lifetime_unit.h"
#include "chrome/browser/resource_coordinator/time.h"
#include "chrome/browser/ui/tabs/tab_strip_model_impl.h"
#include "chrome/browser/ui/tabs/test_tab_strip_model_delegate.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/navigation_controller.h"
#include "content/test/test_web_contents.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

namespace {

constexpr bool kUserGestureTrue = true;
constexpr int kTabsPerTabStrip = 3;

class NoUnloadListenerTabStripModelDelegate : public TestTabStripModelDelegate {
 public:
  NoUnloadListenerTabStripModelDelegate() = default;

  bool RunUnloadListenerBeforeClosing(content::WebContents* contents) override {
    // Unload listeners cannot run in unit tests.
    return false;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NoUnloadListenerTabStripModelDelegate);
};

class MockLifetimeUnitSink : public LifetimeUnitSink {
 public:
  MockLifetimeUnitSink() = default;

  MOCK_METHOD1(OnLifetimeUnitCreated, void(LifetimeUnit*));
  MOCK_METHOD1(OnLifetimeUnitDestroyed, void(LifetimeUnit*));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockLifetimeUnitSink);
};

class TabLifetimeUnitSourceTest : public ChromeRenderViewHostTestHarness {
 protected:
  TabLifetimeUnitSourceTest()
      : source_(&sink_), scoped_set_tick_clock_for_testing_(&tick_clock_) {}

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
    tab_strip_->AddObserver(&source_);
    EXPECT_CALL(sink_, OnLifetimeUnitCreated(testing::_))
        .Times(kTabsPerTabStrip);
    for (int i = 0; i < kTabsPerTabStrip; ++i)
      tab_strip_->AppendWebContents(CreateWebContents(), true);
    testing::Mock::VerifyAndClear(&sink_);

    source_.SetFocusedTabStripModelForTesting(tab_strip_.get());
  }

  void TearDown() override {
    EXPECT_CALL(sink_, OnLifetimeUnitDestroyed(testing::_))
        .Times(kTabsPerTabStrip);
    tab_strip_->CloseAllTabs();
    testing::Mock::VerifyAndClear(&sink_);
    tab_strip_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  TabLifetimeUnit* GetLifetimeUnit(TabStripModel* tab_strip_model, int index) {
    return TabLifetimeUnit::FromWebContents(
        tab_strip_model->GetWebContentsAt(index));
  }

 private:
  NoUnloadListenerTabStripModelDelegate tab_strip_model_delegate_;

 protected:
  testing::StrictMock<MockLifetimeUnitSink> sink_;
  TabLifetimeUnitSource source_;
  base::SimpleTestTickClock tick_clock_;
  ScopedSetTickClockForTesting scoped_set_tick_clock_for_testing_;
  std::unique_ptr<TabStripModelImpl> tab_strip_;
  base::ObserverList<TabLifetimeObserver> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TabLifetimeUnitSourceTest);
};

}  // namespace

TEST_F(TabLifetimeUnitSourceTest, SortKey) {
  tab_strip_->ActivateTabAt(0, kUserGestureTrue);
  tick_clock_.Advance(base::TimeDelta::FromSeconds(1));
  tab_strip_->ActivateTabAt(1, kUserGestureTrue);
  tick_clock_.Advance(base::TimeDelta::FromSeconds(1));
  tab_strip_->ActivateTabAt(2, kUserGestureTrue);

  EXPECT_FALSE(GetLifetimeUnit(tab_strip_.get(), 0)->GetSortKey() <
               GetLifetimeUnit(tab_strip_.get(), 0)->GetSortKey());
  EXPECT_TRUE(GetLifetimeUnit(tab_strip_.get(), 0)->GetSortKey() <
              GetLifetimeUnit(tab_strip_.get(), 1)->GetSortKey());
  EXPECT_TRUE(GetLifetimeUnit(tab_strip_.get(), 0)->GetSortKey() <
              GetLifetimeUnit(tab_strip_.get(), 2)->GetSortKey());

  EXPECT_FALSE(GetLifetimeUnit(tab_strip_.get(), 1)->GetSortKey() <
               GetLifetimeUnit(tab_strip_.get(), 0)->GetSortKey());
  EXPECT_FALSE(GetLifetimeUnit(tab_strip_.get(), 1)->GetSortKey() <
               GetLifetimeUnit(tab_strip_.get(), 1)->GetSortKey());
  EXPECT_TRUE(GetLifetimeUnit(tab_strip_.get(), 1)->GetSortKey() <
              GetLifetimeUnit(tab_strip_.get(), 2)->GetSortKey());

  EXPECT_FALSE(GetLifetimeUnit(tab_strip_.get(), 2)->GetSortKey() <
               GetLifetimeUnit(tab_strip_.get(), 0)->GetSortKey());
  EXPECT_FALSE(GetLifetimeUnit(tab_strip_.get(), 2)->GetSortKey() <
               GetLifetimeUnit(tab_strip_.get(), 1)->GetSortKey());
  EXPECT_FALSE(GetLifetimeUnit(tab_strip_.get(), 2)->GetSortKey() <
               GetLifetimeUnit(tab_strip_.get(), 2)->GetSortKey());

  EXPECT_FALSE(GetLifetimeUnit(tab_strip_.get(), 0)->GetSortKey() >
               GetLifetimeUnit(tab_strip_.get(), 0)->GetSortKey());
  EXPECT_FALSE(GetLifetimeUnit(tab_strip_.get(), 0)->GetSortKey() >
               GetLifetimeUnit(tab_strip_.get(), 1)->GetSortKey());
  EXPECT_FALSE(GetLifetimeUnit(tab_strip_.get(), 0)->GetSortKey() >
               GetLifetimeUnit(tab_strip_.get(), 2)->GetSortKey());

  EXPECT_TRUE(GetLifetimeUnit(tab_strip_.get(), 1)->GetSortKey() >
              GetLifetimeUnit(tab_strip_.get(), 0)->GetSortKey());
  EXPECT_FALSE(GetLifetimeUnit(tab_strip_.get(), 1)->GetSortKey() >
               GetLifetimeUnit(tab_strip_.get(), 1)->GetSortKey());
  EXPECT_FALSE(GetLifetimeUnit(tab_strip_.get(), 1)->GetSortKey() >
               GetLifetimeUnit(tab_strip_.get(), 2)->GetSortKey());

  EXPECT_TRUE(GetLifetimeUnit(tab_strip_.get(), 2)->GetSortKey() >
              GetLifetimeUnit(tab_strip_.get(), 0)->GetSortKey());
  EXPECT_TRUE(GetLifetimeUnit(tab_strip_.get(), 2)->GetSortKey() >
              GetLifetimeUnit(tab_strip_.get(), 1)->GetSortKey());
  EXPECT_FALSE(GetLifetimeUnit(tab_strip_.get(), 2)->GetSortKey() >
               GetLifetimeUnit(tab_strip_.get(), 2)->GetSortKey());
}

TEST_F(TabLifetimeUnitSourceTest, DiscardAndActivate) {
  EXPECT_NE(0, tab_strip_->active_index());
  TabLifetimeUnit* lifetime_unit = GetLifetimeUnit(tab_strip_.get(), 0);

  // Discard a tab.
  EXPECT_EQ(LifetimeUnit::State::ALIVE, lifetime_unit->GetState());
  auto* const initial_web_contents = tab_strip_->GetWebContentsAt(0);
  lifetime_unit->Discard(DiscardCondition::kProactive);
  EXPECT_EQ(LifetimeUnit::State::DISCARDED, lifetime_unit->GetState());
  EXPECT_NE(initial_web_contents, tab_strip_->GetWebContentsAt(0));
  EXPECT_FALSE(tab_strip_->GetWebContentsAt(0)->IsWaitingForResponse());

  // Activate the tab. Expect it to be reloaded.
  tab_strip_->ActivateTabAt(0, kUserGestureTrue);
  EXPECT_EQ(LifetimeUnit::State::ALIVE, lifetime_unit->GetState());
  EXPECT_TRUE(tab_strip_->GetWebContentsAt(0)->IsWaitingForResponse());
}

TEST_F(TabLifetimeUnitSourceTest, ReloadDiscardedTabContextMenu) {
  EXPECT_NE(0, tab_strip_->active_index());
  TabLifetimeUnit* lifetime_unit = GetLifetimeUnit(tab_strip_.get(), 0);

  // Discard a tab.
  EXPECT_EQ(LifetimeUnit::State::ALIVE, lifetime_unit->GetState());
  auto* const initial_web_contents = tab_strip_->GetWebContentsAt(0);
  lifetime_unit->Discard(DiscardCondition::kProactive);
  EXPECT_EQ(LifetimeUnit::State::DISCARDED, lifetime_unit->GetState());
  EXPECT_NE(initial_web_contents, tab_strip_->GetWebContentsAt(0));
  EXPECT_FALSE(tab_strip_->GetWebContentsAt(0)->IsWaitingForResponse());

  // Simulate reloading the tab from the context menu. Don't expect it to remain
  // in the DISCARDED state.
  tab_strip_->GetWebContentsAt(0)->GetController().Reload(
      content::ReloadType::NORMAL, false);
  EXPECT_EQ(LifetimeUnit::State::ALIVE, lifetime_unit->GetState());
  EXPECT_TRUE(tab_strip_->GetWebContentsAt(0)->IsWaitingForResponse());
}

// Verify that the last active time property is saved when the tab is discarded.
TEST_F(TabLifetimeUnitSourceTest, DiscardedTabKeepsLastActiveTime) {
  EXPECT_NE(0, tab_strip_->active_index());
  TabLifetimeUnit* lifetime_unit = GetLifetimeUnit(tab_strip_.get(), 0);

  // Set an old last active time on a tab.
  base::TimeTicks new_last_active_time =
      NowTicks() - base::TimeDelta::FromMinutes(35);
  tab_strip_->GetWebContentsAt(0)->SetLastActiveTime(new_last_active_time);

  // Discard the tab.
  EXPECT_EQ(LifetimeUnit::State::ALIVE, lifetime_unit->GetState());
  auto* const initial_web_contents = tab_strip_->GetWebContentsAt(0);
  lifetime_unit->Discard(DiscardCondition::kProactive);
  EXPECT_EQ(LifetimeUnit::State::DISCARDED, lifetime_unit->GetState());
  EXPECT_NE(initial_web_contents, tab_strip_->GetWebContentsAt(0));
  EXPECT_FALSE(tab_strip_->GetWebContentsAt(0)->IsWaitingForResponse());

  // Verify that the last active time is preserved.
  EXPECT_EQ(new_last_active_time,
            tab_strip_->GetWebContentsAt(0)->GetLastActiveTime());
}

// Verify that CanDiscard() returns false when a tab has already been discarded
// on non-ChromeOS platforms.
TEST_F(TabLifetimeUnitSourceTest, CanOnlyDiscardOnce) {
  EXPECT_NE(0, tab_strip_->active_index());
  TabLifetimeUnit* lifetime_unit = GetLifetimeUnit(tab_strip_.get(), 0);

  // Discard a tab.
  EXPECT_EQ(LifetimeUnit::State::ALIVE, lifetime_unit->GetState());
  auto* const initial_web_contents = tab_strip_->GetWebContentsAt(0);
  lifetime_unit->Discard(DiscardCondition::kProactive);
  EXPECT_EQ(LifetimeUnit::State::DISCARDED, lifetime_unit->GetState());
  EXPECT_NE(initial_web_contents, tab_strip_->GetWebContentsAt(0));
  EXPECT_FALSE(tab_strip_->GetWebContentsAt(0)->IsWaitingForResponse());

  // Reload the tab.
  tab_strip_->GetWebContentsAt(0)->GetController().Reload(
      content::ReloadType::NORMAL, false);
  EXPECT_EQ(LifetimeUnit::State::ALIVE, lifetime_unit->GetState());
  EXPECT_TRUE(tab_strip_->GetWebContentsAt(0)->IsWaitingForResponse());

#if defined(OS_CHROMEOS)
  // On ChromeOS, a tab can be discarded multiple times.
  EXPECT_TRUE(lifetime_unit->CanDiscard(DiscardCondition::kProactive));
#else
  // On non-ChromeOS platforms, a tab cannot be discarded twice.
  EXPECT_FALSE(lifetime_unit->CanDiscard(DiscardCondition::kProactive));
#endif
}

}  // namespace resource_coordinator

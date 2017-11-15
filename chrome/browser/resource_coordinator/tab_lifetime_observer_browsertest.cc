// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/resource_coordinator/tab_lifetime_observer.h"
#include "chrome/browser/resource_coordinator/tab_lifetime_unit.h"
#include "chrome/browser/resource_coordinator/tab_lifetime_unit_source.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"
#include "chrome/browser/resource_coordinator/tab_manager_web_contents_data.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/test_utils.h"
#include "url/gurl.h"

using content::OpenURLParams;
using content::WebContents;

#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_LINUX) || \
    defined(OS_CHROMEOS)

namespace resource_coordinator {

using TabLifetimeObserverTest = InProcessBrowserTest;

class MockTabLifetimeObserver : public TabLifetimeObserver {
 public:
  MockTabLifetimeObserver()
      : nb_events_(0),
        contents_(nullptr),
        is_discarded_(false),
        is_auto_discardable_(true) {}

  // TabLifetimeObserver implementation:
  void OnDiscardedStateChange(content::WebContents* contents,
                              bool is_discarded) override {
    nb_events_++;
    contents_ = contents;
    is_discarded_ = is_discarded;
  }

  void OnAutoDiscardableStateChange(content::WebContents* contents,
                                    bool is_auto_discardable) override {
    nb_events_++;
    contents_ = contents;
    is_auto_discardable_ = is_auto_discardable;
  }

  int nb_events() const { return nb_events_; }
  WebContents* content() const { return contents_; }
  bool is_discarded() const { return is_discarded_; }
  bool is_auto_discardable() const { return is_auto_discardable_; }

 private:
  int nb_events_;
  WebContents* contents_;
  bool is_discarded_;
  bool is_auto_discardable_;

  DISALLOW_COPY_AND_ASSIGN(MockTabLifetimeObserver);
};

IN_PROC_BROWSER_TEST_F(TabLifetimeObserverTest, OnDiscardStateChange) {
  TabManager* tab_manager = g_browser_process->GetTabManager();
  auto* tsm = browser()->tab_strip_model();

  // Open two tabs.
  OpenURLParams open1(GURL(chrome::kChromeUIAboutURL), content::Referrer(),
                      WindowOpenDisposition::CURRENT_TAB,
                      ui::PAGE_TRANSITION_TYPED, false);
  browser()->OpenURL(open1);

  OpenURLParams open2(GURL(chrome::kChromeUICreditsURL), content::Referrer(),
                      WindowOpenDisposition::NEW_BACKGROUND_TAB,
                      ui::PAGE_TRANSITION_TYPED, false);
  browser()->OpenURL(open2);

  ASSERT_EQ(2, tsm->count());

  // Subscribe observer to TabManager's observer list.
  MockTabLifetimeObserver observer;
  tab_manager->AddObserver(&observer);

  // Discards both tabs and make sure the events were observed properly.
  TabLifetimeUnit::FromWebContents(tsm->GetWebContentsAt(0))
      ->Discard(MemoryCondition::NORMAL);
  EXPECT_EQ(1, observer.nb_events());
  EXPECT_EQ(tsm->GetWebContentsAt(0), observer.content());
  EXPECT_TRUE(observer.is_discarded());

  TabLifetimeUnit::FromWebContents(tsm->GetWebContentsAt(1))
      ->Discard(MemoryCondition::NORMAL);
  EXPECT_EQ(2, observer.nb_events());
  EXPECT_EQ(tsm->GetWebContentsAt(1), observer.content());
  EXPECT_TRUE(observer.is_discarded());

  // Discarding an already discarded tab shouldn't fire the observers.
  TabLifetimeUnit::FromWebContents(tsm->GetWebContentsAt(0))
      ->Discard(MemoryCondition::NORMAL);
  EXPECT_EQ(2, observer.nb_events());

  // Reload tab 2.
  tsm->ActivateTabAt(1, false);
  EXPECT_EQ(1, tsm->active_index());
  EXPECT_EQ(3, observer.nb_events());
  EXPECT_EQ(tsm->GetWebContentsAt(1), observer.content());
  EXPECT_FALSE(observer.is_discarded());

  // Reloading a tab that's not discarded shouldn't fire the observers.
  tsm->ActivateTabAt(1, false);
  EXPECT_EQ(3, observer.nb_events());

  // Reload tab 1.
  tsm->ActivateTabAt(0, false);
  EXPECT_EQ(0, tsm->active_index());
  EXPECT_EQ(4, observer.nb_events());
  EXPECT_EQ(tsm->GetWebContentsAt(0), observer.content());
  EXPECT_FALSE(observer.is_discarded());

  // After removing the observer from the TabManager's list, it shouldn't
  // receive events anymore.
  tab_manager->RemoveObserver(&observer);
  TabLifetimeUnit::FromWebContents(tsm->GetWebContentsAt(1))
      ->Discard(MemoryCondition::NORMAL);
  EXPECT_EQ(4, observer.nb_events());
}

IN_PROC_BROWSER_TEST_F(TabLifetimeObserverTest, OnAutoDiscardableStateChange) {
  TabManager* tab_manager = g_browser_process->GetTabManager();
  auto* tsm = browser()->tab_strip_model();

  // Open two tabs.
  OpenURLParams open(GURL(chrome::kChromeUIAboutURL), content::Referrer(),
                     WindowOpenDisposition::NEW_BACKGROUND_TAB,
                     ui::PAGE_TRANSITION_TYPED, false);
  WebContents* contents = browser()->OpenURL(open);

  // Subscribe observer to TabManager's observer list.
  MockTabLifetimeObserver observer;
  tab_manager->AddObserver(&observer);

  // No events initially.
  EXPECT_EQ(0, observer.nb_events());

  // Should maintain at zero since the default value of the state is true.
  TabLifetimeUnit::FromWebContents(tsm->GetWebContentsAt(1))
      ->SetAutoDiscardable(true);
  EXPECT_EQ(0, observer.nb_events());

  // Now it has to change.
  TabLifetimeUnit::FromWebContents(tsm->GetWebContentsAt(1))
      ->SetAutoDiscardable(false);
  EXPECT_EQ(1, observer.nb_events());
  EXPECT_FALSE(observer.is_auto_discardable());
  EXPECT_EQ(contents, observer.content());

  // No changes since it's not a new state.
  TabLifetimeUnit::FromWebContents(tsm->GetWebContentsAt(1))
      ->SetAutoDiscardable(false);
  EXPECT_EQ(1, observer.nb_events());

  // Change it back and we should have another event.
  TabLifetimeUnit::FromWebContents(tsm->GetWebContentsAt(1))
      ->SetAutoDiscardable(true);
  EXPECT_EQ(2, observer.nb_events());
  EXPECT_TRUE(observer.is_auto_discardable());
  EXPECT_EQ(contents, observer.content());
}

}  // namespace resource_coordinator

#endif  // defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_LINUX) ||
        // defined(OS_CHROMEOS)

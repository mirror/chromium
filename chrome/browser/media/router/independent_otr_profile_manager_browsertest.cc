// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <set>

#include "base/run_loop.h"
#include "base/stl_util.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/media/router/independent_otr_profile_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

namespace content {
class NotificationDetails;
}  // namespace content

namespace {

class ProfileDestructionWatcher final : public content::NotificationObserver {
 public:
  using DestructionCallback = base::OnceCallback<void()>;

  ProfileDestructionWatcher() = default;
  ~ProfileDestructionWatcher() override = default;

  void Watch(Profile* profile, DestructionCallback callback) {
    profile_ = profile;
    callback_ = std::move(callback);
    registrar_.Add(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
                   content::Source<Profile>(profile_));
  }

  // content::NotificationObserver overrides.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    DCHECK_EQ(chrome::NOTIFICATION_PROFILE_DESTROYED, type);
    DCHECK(content::Source<Profile>(profile_) == source);
    std::move(callback_).Run();
  }

 private:
  Profile* profile_;
  DestructionCallback callback_;
  content::NotificationRegistrar registrar_;
};

// Profile actually avoids sending more than one destroyed notification; this
// is just a precaution against the memory being written to (and resetting the
// flag Profile uses) between a first and second free, thereby causing two
// notifications.  As a result, the tests should still be written to place
// ProfileDestructionWatcher as late as possible.
class DoubleDeleteWatcher final : public content::NotificationObserver {
 public:
  DoubleDeleteWatcher() = default;
  ~DoubleDeleteWatcher() override = default;

  void Watch() {
    registrar_.Add(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
                   content::NotificationService::AllSources());
  }

  // content::NotificationObserver overrides.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    DCHECK_EQ(chrome::NOTIFICATION_PROFILE_DESTROYED, type);
    ASSERT_FALSE(base::ContainsKey(deleted_profiles_, source.map_key()));
    deleted_profiles_.insert(source.map_key());
  }

 private:
  std::set<uintptr_t> deleted_profiles_;
  content::NotificationRegistrar registrar_;
};

class IndependentOTRProfileManagerTest : public InProcessBrowserTest {
 protected:
  void SetUpOnMainThread() override {
    double_delete_watcher_.Watch();
    InProcessBrowserTest::SetUpOnMainThread();
  }

  void VerifyBrowserIsNew(Browser* old, Browser* next) {
    ASSERT_NE(nullptr, next);
    ASSERT_NE(old, next);
  }

  Browser* CreateBrowserForProfile(Profile* profile) {
    auto params =
        chrome::NavigateParams(profile, GURL("about:blank"),
                               ui::PageTransition::PAGE_TRANSITION_FIRST);
    params.disposition = WindowOpenDisposition::NEW_POPUP;
    chrome::Navigate(&params);
    VerifyBrowserIsNew(browser(), params.browser);
    return params.browser;
  }

  DoubleDeleteWatcher double_delete_watcher_;
  IndependentOTRProfileManager* manager_ =
      IndependentOTRProfileManager::GetInstance();
};

}  // namespace

IN_PROC_BROWSER_TEST_F(IndependentOTRProfileManagerTest, CreateAndDestroy) {
  ProfileDestructionWatcher watcher;
  base::RunLoop run_loop;
  {
    auto profile_registration =
        manager_->CreateFromOriginalProfile(browser()->profile());
    auto* otr_profile = profile_registration.profile();

    ASSERT_NE(browser()->profile(), otr_profile);
    EXPECT_NE(browser()->profile()->GetOffTheRecordProfile(), otr_profile);
    EXPECT_TRUE(otr_profile->IsOffTheRecord());

    watcher.Watch(otr_profile, run_loop.QuitClosure());
  }

  // |watcher| will trigger Quit() when the OTR profile is destroyed.
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(IndependentOTRProfileManagerTest, RegistrationMove) {
  ProfileDestructionWatcher watcher;
  base::RunLoop run_loop;

  auto profile_registration1 =
      manager_->CreateFromOriginalProfile(browser()->profile());
  {
    auto profile_registration2(std::move(profile_registration1));
    EXPECT_EQ(nullptr, profile_registration1.profile());
    watcher.Watch(profile_registration2.profile(), run_loop.QuitClosure());
  }

  // |watcher| will trigger Quit() when the OTR profile is destroyed.
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(IndependentOTRProfileManagerTest,
                       DeleteWaitsForLastBrowser) {
  ProfileDestructionWatcher watcher;
  Profile* otr_profile = nullptr;
  Browser* browser1 = nullptr;
  Browser* browser2 = nullptr;
  {
    auto profile_registration =
        manager_->CreateFromOriginalProfile(browser()->profile());
    otr_profile = profile_registration.profile();

    browser1 = CreateBrowserForProfile(otr_profile);
    browser2 = CreateBrowserForProfile(otr_profile);
    ASSERT_NE(browser1, browser2);
  }

  browser1->window()->Close();
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(base::ContainsValue(*BrowserList::GetInstance(), browser1));
  ASSERT_TRUE(base::ContainsValue(*BrowserList::GetInstance(), browser2));

  base::RunLoop run_loop;
  watcher.Watch(otr_profile, run_loop.QuitClosure());
  browser2->window()->Close();
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(IndependentOTRProfileManagerTest,
                       DeleteImmediatelyWhenBrowsersAlreadyClosed) {
  ProfileDestructionWatcher watcher;
  base::RunLoop run_loop;
  {
    auto profile_registration =
        manager_->CreateFromOriginalProfile(browser()->profile());
    auto* otr_profile = profile_registration.profile();

    auto* browser1 = CreateBrowserForProfile(otr_profile);
    auto* browser2 = CreateBrowserForProfile(otr_profile);
    ASSERT_NE(browser1, browser2);

    browser1->window()->Close();
    browser2->window()->Close();
    base::RunLoop().RunUntilIdle();
    ASSERT_FALSE(base::ContainsValue(*BrowserList::GetInstance(), browser1));
    ASSERT_FALSE(base::ContainsValue(*BrowserList::GetInstance(), browser2));

    watcher.Watch(otr_profile, run_loop.QuitClosure());
  }

  // |watcher| will trigger Quit() when the OTR profile is destroyed.
  run_loop.Run();
}

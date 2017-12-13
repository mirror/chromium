// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <set>

#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/threading/thread_restrictions.h"
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
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

namespace content {
class NotificationDetails;
}  // namespace content

namespace {

class ProfileDestructionWatcher final : public content::NotificationObserver {
 public:
  using DestructionCallback = base::OnceClosure;

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

void OriginalProfileNeverDestroyed(Profile* profile) {
  FAIL()
      << "Original profile unexpectedly destroyed before dependent OTR profile";
}

// This class acts as an owner of an OTRProfileRegistration and deletes the
// registration when it is notified that the original profile is being
// destroyed.  This is the minimum behavior expected by owners of
// OTRProfileRegistration.
class RegistrationOwner {
 public:
  RegistrationOwner(IndependentOTRProfileManager* manager, Profile* profile)
      : otr_profile_registration_(manager->CreateFromOriginalProfile(
            profile,
            base::BindOnce(&RegistrationOwner::OriginalProfileDestroyed,
                           base::Unretained(this)))) {}

  Profile* profile() const { return otr_profile_registration_->profile(); }

 private:
  void OriginalProfileDestroyed(Profile* profile) {
    DCHECK(profile == otr_profile_registration_->profile());
    otr_profile_registration_.reset();
  }

  std::unique_ptr<IndependentOTRProfileManager::OTRProfileRegistration>
      otr_profile_registration_;
};

class IndependentOTRProfileManagerTest : public InProcessBrowserTest {
 protected:
  IndependentOTRProfileManager* manager_ =
      IndependentOTRProfileManager::GetInstance();
};

}  // namespace

IN_PROC_BROWSER_TEST_F(IndependentOTRProfileManagerTest, CreateAndDestroy) {
  ProfileDestructionWatcher watcher;
  base::RunLoop run_loop;
  {
    auto profile_registration = manager_->CreateFromOriginalProfile(
        browser()->profile(), base::BindOnce(&OriginalProfileNeverDestroyed));
    auto* otr_profile = profile_registration->profile();

    ASSERT_NE(browser()->profile(), otr_profile);
    EXPECT_NE(browser()->profile()->GetOffTheRecordProfile(), otr_profile);
    EXPECT_TRUE(otr_profile->IsOffTheRecord());

    watcher.Watch(otr_profile, run_loop.QuitClosure());
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
    auto profile_registration = manager_->CreateFromOriginalProfile(
        browser()->profile(), base::BindOnce(&OriginalProfileNeverDestroyed));
    otr_profile = profile_registration->profile();

    browser1 = CreateBrowser(otr_profile);
    browser2 = CreateBrowser(otr_profile);
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
    auto profile_registration = manager_->CreateFromOriginalProfile(
        browser()->profile(), base::BindOnce(&OriginalProfileNeverDestroyed));
    auto* otr_profile = profile_registration->profile();

    auto* browser1 = CreateBrowser(otr_profile);
    auto* browser2 = CreateBrowser(otr_profile);
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

IN_PROC_BROWSER_TEST_F(IndependentOTRProfileManagerTest,
                       CreateTwoFromSameProfile) {
  ProfileDestructionWatcher watcher1;
  ProfileDestructionWatcher watcher2;
  base::RunLoop run_loop1;
  base::RunLoop run_loop2;
  {
    auto profile_registration1 = manager_->CreateFromOriginalProfile(
        browser()->profile(), base::BindOnce(&OriginalProfileNeverDestroyed));
    auto* otr_profile1 = profile_registration1->profile();

    auto profile_registration2 = manager_->CreateFromOriginalProfile(
        browser()->profile(), base::BindOnce(&OriginalProfileNeverDestroyed));
    auto* otr_profile2 = profile_registration2->profile();

    ASSERT_NE(otr_profile1, otr_profile2);

    watcher1.Watch(otr_profile1, run_loop1.QuitClosure());
    watcher2.Watch(otr_profile2, run_loop2.QuitClosure());
  }

  run_loop1.Run();
  run_loop2.Run();
}

//  x original destroyed destroys dependent otr profile
//  - otr with browser, browser destroyed, then original
//    - notification observer should still live and cause last otr to be
//      destroyed
IN_PROC_BROWSER_TEST_F(IndependentOTRProfileManagerTest,
                       OriginalProfileDestroyedFirst) {
  ProfileDestructionWatcher watcher;
  base::RunLoop run_loop;

  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  auto original_profile = base::WrapUnique(Profile::CreateProfile(
      temp_dir.GetPath(), nullptr, Profile::CREATE_MODE_SYNCHRONOUS));
  ASSERT_TRUE(original_profile);
  auto profile_owner = RegistrationOwner(manager_, original_profile.get());
  auto* otr_profile = profile_owner.profile();

  ASSERT_NE(original_profile.get(), otr_profile);
  EXPECT_NE(original_profile->GetOffTheRecordProfile(), otr_profile);

  watcher.Watch(otr_profile, run_loop.QuitClosure());
  original_profile.reset();
  // |original_profile| being destroyed should trigger the dependent OTR
  // profile to be destroyed and quit this RunLoop via |watcher|.
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(IndependentOTRProfileManagerTest,
                       OriginalProfileDestroyedFirstTwoOTR) {
  ProfileDestructionWatcher watcher1;
  ProfileDestructionWatcher watcher2;
  base::RunLoop run_loop1;
  base::RunLoop run_loop2;

  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  auto original_profile = base::WrapUnique(Profile::CreateProfile(
      temp_dir.GetPath(), nullptr, Profile::CREATE_MODE_SYNCHRONOUS));
  ASSERT_TRUE(original_profile);
  auto profile_owner1 = RegistrationOwner(manager_, original_profile.get());
  auto* otr_profile1 = profile_owner1.profile();

  auto profile_owner2 = RegistrationOwner(manager_, original_profile.get());
  auto* otr_profile2 = profile_owner2.profile();

  watcher1.Watch(otr_profile1, run_loop1.QuitClosure());
  watcher2.Watch(otr_profile2, run_loop2.QuitClosure());
  original_profile.reset();
  // |original_profile| being destroyed should trigger the dependent OTR
  // profiles to be destroyed and quit both RunLoops via |watcher1| and
  // |watcher2|.
  run_loop1.Run();
  run_loop2.Run();
}

IN_PROC_BROWSER_TEST_F(IndependentOTRProfileManagerTest,
                       BrowserClosingDoesntRemoveProfileObserver) {
  ProfileDestructionWatcher watcher1;
  ProfileDestructionWatcher watcher2;
  base::RunLoop run_loop1;
  base::RunLoop run_loop2;

  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  auto original_profile = base::WrapUnique(Profile::CreateProfile(
      temp_dir.GetPath(), nullptr, Profile::CREATE_MODE_SYNCHRONOUS));
  ASSERT_TRUE(original_profile);
  auto profile_owner1 = RegistrationOwner(manager_, original_profile.get());
  auto* otr_profile1 = profile_owner1.profile();
  Browser* browser = nullptr;
  {
    auto profile_owner2 = RegistrationOwner(manager_, original_profile.get());
    auto* otr_profile2 = profile_owner2.profile();

    browser = CreateBrowser(otr_profile2);
    watcher2.Watch(otr_profile2, run_loop2.QuitClosure());
  }
  browser->window()->Close();
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(base::ContainsValue(*BrowserList::GetInstance(), browser));
  run_loop2.Run();

  watcher1.Watch(otr_profile1, run_loop1.QuitClosure());
  original_profile.reset();
  run_loop1.Run();
}

IN_PROC_BROWSER_TEST_F(IndependentOTRProfileManagerTest,
                       CallbackNotCalledAfterUnregister) {
  ProfileDestructionWatcher watcher;
  base::RunLoop run_loop;

  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  auto original_profile = base::WrapUnique(Profile::CreateProfile(
      temp_dir.GetPath(), nullptr, Profile::CREATE_MODE_SYNCHRONOUS));
  ASSERT_TRUE(original_profile);
  Browser* browser = nullptr;
  Profile* otr_profile = nullptr;
  {
    auto profile_registration = manager_->CreateFromOriginalProfile(
        original_profile.get(), base::BindOnce(&OriginalProfileNeverDestroyed));
    otr_profile = profile_registration->profile();

    browser = CreateBrowser(otr_profile);
  }
  watcher.Watch(otr_profile, run_loop.QuitClosure());
  browser->window()->Close();
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(base::ContainsValue(*BrowserList::GetInstance(), browser));

  original_profile.reset();
  // |original_profile| being destroyed should trigger the dependent OTR
  // profile to be destroyed and quit this RunLoop via |watcher|.
  run_loop.Run();
}

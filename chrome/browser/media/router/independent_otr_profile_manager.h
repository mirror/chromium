// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_INDEPENDENT_OTR_PROFILE_MANAGER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_INDEPENDENT_OTR_PROFILE_MANAGER_H_

#include <cstdint>
#include <memory>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class Profile;

// This class manages Profile instances that were created from
// Profile::CreateOffTheRecordProfile().  These instances aren't owned by the
// original Profile object and so the lifetime assumptions are different.
//
// Normally, a Browser object will delete an OTR profile if it is the last one
// using it, and this deletion occurs via the original profile.  Users of
// CreateOffTheRecordProfile can't let Browser take the same destruction path
// and must instead rely on this class.
//
// In particular, this functionality is used by offscreen tabs and presentation
// API receiver windows.  In the case of offscreen tabs, there is no interaction
// with a Browser object outside of DevTools.  The presentation API windows
// don't necessarily have special lifetime requirements but they still can't use
// the same destruction path in ~Browser as the normal OTR profile.  DevTools
// presents a similar problem for presentation API windows as well.
//
// All methods must be called on the UI thread.
class IndependentOTRProfileManager final
    : public chrome::BrowserListObserver,
      public content::NotificationObserver {
 public:
  class OTRProfileRegistration {
   public:
    ~OTRProfileRegistration();

    Profile* profile() const { return profile_; }

   private:
    friend class IndependentOTRProfileManager;

    OTRProfileRegistration(IndependentOTRProfileManager* manager,
                           Profile* profile);

    IndependentOTRProfileManager* const manager_;
    Profile* const profile_;

    DISALLOW_COPY_AND_ASSIGN(OTRProfileRegistration);
  };

  using ProfileDestroyedCallback = base::OnceCallback<void(Profile*)>;

  static IndependentOTRProfileManager* GetInstance();

  // |callback| will be called if |profile| is being destroyed.  If |callback|
  // is called, the registration this method returns should be destroyed.
  std::unique_ptr<OTRProfileRegistration> CreateFromOriginalProfile(
      Profile* profile,
      ProfileDestroyedCallback callback);

 private:
  IndependentOTRProfileManager();
  ~IndependentOTRProfileManager() final;

  bool HasDependentProfiles(Profile* profile) const;
  void UnregisterProfile(Profile* profile);

  // chrome::BrowserListObserver overrides.
  void OnBrowserAdded(Browser* browser) final;
  void OnBrowserRemoved(Browser* browser) final;

  // content::NotificationObserver overrides.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) final;

  // Counts the number of Browser instances referencing an independent OTR
  // profile plus 1 for the OTRProfileRegistration object that created it.
  base::flat_map<Profile*, int32_t> refcounts_;
  base::flat_map<Profile*, ProfileDestroyedCallback> callbacks_;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(IndependentOTRProfileManager);
};

#endif  // CHROME_BROWSER_MEDIA_ROUTER_INDEPENDENT_OTR_PROFILE_MANAGER_H_

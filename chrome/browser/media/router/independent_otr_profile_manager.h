// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_INDEPENDENT_OTR_PROFILE_MANAGER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_INDEPENDENT_OTR_PROFILE_MANAGER_H_

#include <cstdint>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "chrome/browser/ui/browser_list_observer.h"

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
class IndependentOTRProfileManager : public chrome::BrowserListObserver {
 public:
  class OTRProfileRegistration {
   public:
    OTRProfileRegistration(OTRProfileRegistration&& other);
    OTRProfileRegistration& operator=(OTRProfileRegistration other);
    ~OTRProfileRegistration();

    Profile* profile() const { return profile_; }

   private:
    friend class IndependentOTRProfileManager;

    OTRProfileRegistration(IndependentOTRProfileManager* manager,
                           Profile* profile);

    IndependentOTRProfileManager* manager_;
    Profile* profile_;
  };

  static IndependentOTRProfileManager* GetInstance();

  OTRProfileRegistration CreateFromOriginalProfile(Profile* profile);

  bool IsIndependentOTRProfile(Profile* profile) const;

  // chrome::BrowserListObserver overrides.
  void OnBrowserAdded(Browser* browser) override;
  void OnBrowserRemoved(Browser* browser) override;

 private:
  IndependentOTRProfileManager();
  ~IndependentOTRProfileManager() override;

  void UnregisterProfile(Profile* profile);

  base::flat_map<Profile*, int32_t> refcounts_;

  DISALLOW_COPY_AND_ASSIGN(IndependentOTRProfileManager);
};

#endif  // CHROME_BROWSER_MEDIA_ROUTER_INDEPENDENT_OTR_PROFILE_MANAGER_H_

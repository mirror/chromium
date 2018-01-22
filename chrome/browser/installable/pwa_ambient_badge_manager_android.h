// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INSTALLABLE_PWA_AMBIENT_BADGE_MANAGER_ANDROID_H_
#define CHROME_BROWSER_INSTALLABLE_PWA_AMBIENT_BADGE_MANAGER_ANDROID_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/gurl.h"

struct InstallableData;
class InstallableManager;

// Tab helper to show the PWA ambient badge.
class PwaAmbientBadgeManagerAndroid
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PwaAmbientBadgeManagerAndroid> {
 public:
  ~PwaAmbientBadgeManagerAndroid() override;

 private:
  explicit PwaAmbientBadgeManagerAndroid(content::WebContents* contents);
  friend class content::WebContentsUserData<PwaAmbientBadgeManagerAndroid>;

  void OnDidFinishInstallableCheck(const InstallableData& data);

  // content::WebContentsObserver overrides.
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  InstallableManager* installable_manager_;
  base::WeakPtrFactory<PwaAmbientBadgeManagerAndroid> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PwaAmbientBadgeManagerAndroid);
};

#endif  // CHROME_BROWSER_INSTALLABLE_PWA_AMBIENT_BADGE_MANAGER_ANDROID_H_

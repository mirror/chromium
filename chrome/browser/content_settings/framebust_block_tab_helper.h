// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONTENT_SETTINGS_FRAMEBUST_BLOCK_TAB_HELPER_H_
#define CHROME_BROWSER_CONTENT_SETTINGS_FRAMEBUST_BLOCK_TAB_HELPER_H_

#include <vector>

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

// A tab helper that keeps track of blocked framebust that happened on each
// page.
class FramebustBlockTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<FramebustBlockTabHelper> {
 public:
  class Observer {
   public:
    // Called when a new blocked redirection happened.
    virtual void OnBlockedUrlAdded(const GURL& blocked_url) = 0;

   protected:
    virtual ~Observer() = default;
  };

  ~FramebustBlockTabHelper() override;

  // Shows the Framebust block icon in the Omnibox for the |blocked_url|.
  // If it is already visible, adds |blocked_url| to the list of current blocked
  // redirections and updates the bubble view.
  void AddBlockedUrl(const GURL& blocked_url);

  // Returns the list of current blocked redirections.
  const std::vector<GURL>& GetBlockedUrls();

  // Sets and clears the current observer.
  void SetObserver(Observer* observer);
  void ClearObserver();

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

 private:
  friend class content::WebContentsUserData<FramebustBlockTabHelper>;

  explicit FramebustBlockTabHelper(content::WebContents* web_contents);

  // Remembers all the currently blocked urls. This is cleared on each
  // navigations.
  std::vector<GURL> blocked_urls_;

  // Points to the unique observer of this class.
  Observer* observer_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(FramebustBlockTabHelper);
};

#endif  // CHROME_BROWSER_CONTENT_SETTINGS_FRAMEBUST_BLOCK_TAB_HELPER_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/bookmark_app_error.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/profiles/profile.h"


namespace chrome {

void BookmarkAppError::OpenInChrome() {
  Browser* app_browser = chrome::FindBrowserWithWebContents(web_contents_);

  TabStripModel* tab_strip = app_browser->tab_strip_model();
  tab_strip->DetachWebContentsAt(tab_strip->active_index());

  Profile* profile = Profile::FromBrowserContext(web_contents_->GetBrowserContext());
  Browser* browser = chrome::FindTabbedBrowser(profile, false);
  browser = browser ? browser
                 : new Browser(Browser::CreateParams(profile, true));

  browser->tab_strip_model()->AppendWebContents(web_contents_, true);
  browser->window()->Show();
}

BookmarkAppError::BookmarkAppError(content::WebContents* web_contents)
    : web_contents_(web_contents) {
}

BookmarkAppError::~BookmarkAppError() {
}

}  // namespace chrome

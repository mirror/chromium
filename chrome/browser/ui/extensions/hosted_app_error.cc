// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/extensions/hosted_app_error.h"

#include <utility>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

namespace extensions {

void HostedAppError::Proceed() {
  Browser* app_browser = chrome::FindBrowserWithWebContents(web_contents());

  TabStripModel* tab_strip = app_browser->tab_strip_model();
  tab_strip->DetachWebContentsAt(tab_strip->active_index());

  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  Browser* browser = chrome::FindTabbedBrowser(profile, false);
  if (!browser)
    browser =
        new Browser(Browser::CreateParams(profile, true /* user_gesture */));

  browser->tab_strip_model()->AppendWebContents(web_contents(),
                                                true /* foreground */);
  browser->window()->Show();
  std::move(on_web_contents_reparented_).Run();
}

HostedAppError::HostedAppError(
    content::WebContents* web_contents,
    OnWebContentsReparentedCallback on_web_contents_reparented)
    : web_contents_(web_contents),
      on_web_contents_reparented_(std::move(on_web_contents_reparented)) {}

HostedAppError::~HostedAppError() {}

}  // namespace extensions

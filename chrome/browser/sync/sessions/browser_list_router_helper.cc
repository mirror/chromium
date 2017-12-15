// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/sessions/browser_list_router_helper.h"

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/sync/tab_contents_synced_tab_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/notification_service.h"

namespace sync_sessions {

BrowserListRouterHelper::BrowserListRouterHelper(
    SyncSessionsWebContentsRouter* router,
    Profile* profile)
    : router_(router), profile_(profile) {
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list) {
    if (browser->profile() == profile_) {
      browser->tab_strip_model()->AddObserver(this);
    }
  }
  browser_list->AddObserver(this);

  registrar_.Add(this, chrome::NOTIFICATION_BROWSER_OPENED,
                 content::NotificationService::AllBrowserContextsAndSources());
  registrar_.Add(this, chrome::NOTIFICATION_CLOSE_ALL_BROWSERS_REQUEST,
                 content::NotificationService::AllBrowserContextsAndSources());
  registrar_.Add(this, chrome::NOTIFICATION_BROWSER_CLOSE_CANCELLED,
                 content::NotificationService::AllSources());
}

BrowserListRouterHelper::~BrowserListRouterHelper() {
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list) {
    if (browser->profile() == profile_) {
      browser->tab_strip_model()->RemoveObserver(this);
    }
  }

  BrowserList::GetInstance()->RemoveObserver(this);
}

void BrowserListRouterHelper::OnBrowserAdded(Browser* browser) {
  if (browser->profile() == profile_) {
    browser->tab_strip_model()->AddObserver(this);
  }
}

void BrowserListRouterHelper::OnBrowserRemoved(Browser* browser) {
  if (browser->profile() == profile_) {
    browser->tab_strip_model()->RemoveObserver(this);
  }
}

void BrowserListRouterHelper::OnBrowserClosing(Browser* browser) {
  if (browser->profile() == profile_ &&
      chrome::GetBrowserCount(profile_) == 1U) {
    router_->SetAllBrowsersClosing(true);
  }
}

void BrowserListRouterHelper::TabInsertedAt(TabStripModel* model,
                                            content::WebContents* web_contents,
                                            int index,
                                            bool foreground) {
  if (web_contents && Profile::FromBrowserContext(
                          web_contents->GetBrowserContext()) == profile_)
    router_->NotifyTabModified(web_contents, false);
}

void BrowserListRouterHelper::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_CLOSE_ALL_BROWSERS_REQUEST: {
      router_->SetAllBrowsersClosing(true);
      break;
    }
    case chrome::NOTIFICATION_BROWSER_OPENED:
    case chrome::NOTIFICATION_BROWSER_CLOSE_CANCELLED: {
      router_->SetAllBrowsersClosing(false);
      break;
    }
    default: {
      NOTREACHED();
      break;
    }
  }
}

}  // namespace sync_sessions

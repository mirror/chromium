// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/mru_browser_enumerator.h"

#include <algorithm>

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"

namespace resource_coordinator {
namespace {

// Helper function to create a BrowserInfo from a Browser.
BrowserInfo GetBrowserInfo(const Browser* browser) {
  BrowserInfo browser_info;
  browser_info.browser = browser;
  browser_info.tab_strip_model = browser->tab_strip_model();
  browser_info.window_is_minimized = browser->window()->IsMinimized();
  browser_info.browser_is_app = browser->is_app();
  return browser_info;
}

}  // namespace

MRUBrowserEnumerator::MRUBrowserEnumerator() = default;
MRUBrowserEnumerator::~MRUBrowserEnumerator() = default;

std::vector<BrowserInfo> MRUBrowserEnumerator::GetBrowserInfoList() const {
  BrowserList* browser_list = BrowserList::GetInstance();
  std::vector<BrowserInfo> browser_info_list(browser_list->size());

  // Populate |browser_info_list| in last-active order.
  std::transform(browser_list->begin_last_active(),
                 browser_list->end_last_active(), browser_info_list.begin(),
                 GetBrowserInfo);
  return browser_info_list;
}

base::Optional<BrowserInfo> MRUBrowserEnumerator::GetActiveBrowserInfo() const {
  const Browser* last_active = BrowserList::GetInstance()->GetLastActive();
  if (!last_active)
    return base::nullopt;
  return base::Optional<BrowserInfo>(GetBrowserInfo(last_active));
}

}  // namespace resource_coordinator

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/activity_services/share_to_data_builder.h"

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/tabs/tab.h"
#include "ios/chrome/browser/ui/activity_services/chrome_activity_item_thumbnail_generator.h"
#include "ios/chrome/browser/ui/activity_services/share_to_data.h"
#import "ios/web/public/navigation_item.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

NSString* GetOriginalTitle(const web::NavigationItem* item) {
  if (!item)
    return nil;
  base::string16 pageTitle = item->GetTitle();
  return pageTitle.empty() ? nil : base::SysUTF16ToNSString(pageTitle);
}
}

namespace activity_services {

ShareToData* ShareToDataForTab(Tab* tab, const web::WebState* web_state) {
  // The crash documented in crbug.com/503955 occurs when accessing tab.url
  // (which is a reference parameter).  This suggests that |tab| may be
  // deallocated along the way, in which case its corresponding WebState would
  // be nil.  Test for this and return early to avoid the crash.
  if (!web_state)
    return nil;

  NSString* title = base::SysUTF16ToNSString(web_state->GetTitle());
  if (![title length])
    title = l10n_util::GetNSString(IDS_DEFAULT_TAB_TITLE);

  const web::NavigationItem* lastCommittedItem =
      web_state->GetNavigationManager()->GetLastCommittedItem();
  NSString* originalTitle = GetOriginalTitle(lastCommittedItem);

  BOOL isPagePrintable = [tab viewForPrinting] != nil;
  ThumbnailGeneratorBlock thumbnailGenerator =
      activity_services::ThumbnailGeneratorForTab(tab);
  return [[ShareToData alloc] initWithURL:web_state->GetVisibleURL()
                                    title:title
                          isOriginalTitle:(originalTitle != nil)
                          isPagePrintable:isPagePrintable
                       thumbnailGenerator:thumbnailGenerator];
}

}  // namespace activity_services

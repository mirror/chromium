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
  base::string16 page_title = item->GetTitle();
  return page_title.empty() ? nil : base::SysUTF16ToNSString(page_title);
}
}

namespace activity_services {

ShareToData* ShareToDataForTab(Tab* tab) {
  web::WebState* web_state = tab.webState;

  // For crash documented in crbug.com/503955, tab.url which is being passed
  // as a reference parameter caused a crash due to invalid address which
  // which suggests that |tab| may be deallocated along the way. Check that
  // tab is still valid by checking webState which would be deallocated if
  // tab is being closed.
  if (!web_state)
    return nil;

  NSString* title = base::SysUTF16ToNSString(web_state->GetTitle());
  if (![title length])
    title = l10n_util::GetNSString(IDS_DEFAULT_TAB_TITLE);

  const web::NavigationItem* last_committed_item =
      web_state->GetNavigationManager()->GetLastCommittedItem();
  NSString* original_title = GetOriginalTitle(last_committed_item);

  BOOL is_page_printable = [tab viewForPrinting] != nil;
  ThumbnailGeneratorBlock thumbnail_generator =
      activity_services::ThumbnailGeneratorForTab(tab);
  return [[ShareToData alloc] initWithURL:web_state->GetVisibleURL()
                                    title:title
                          isOriginalTitle:(original_title != nil)
                          isPagePrintable:is_page_printable
                       thumbnailGenerator:thumbnail_generator];
}

}  // namespace activity_services

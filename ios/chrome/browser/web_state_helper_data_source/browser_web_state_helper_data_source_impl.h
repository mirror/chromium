// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_IMPL_H_
#define IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_IMPL_H_

#import "ios/chrome/browser/ui/browser_list/browser_list_observer.h"
#import "ios/chrome/browser/web_state_helper_data_source/browser_web_state_helper_data_source.h"

class BrowserList;

// Concrete subclass of BrowserWebStateHelperDataSource.
class BrowserWebStateHelperDataSourceImpl
    : public BrowserListObserver,
      public BrowserWebStateHelperDataSource {
 public:
  BrowserWebStateHelperDataSourceImpl();
  ~BrowserWebStateHelperDataSourceImpl() override;

 private:
  // BrowserListObserver:
  void OnBrowserCreated(BrowserList* browser_list, Browser* browser) override;

  // BrowserWebStateHelperDataSource:
  void ProvideHelpersToBrowsers(BrowserList* browser_list) override;

  // KeyedService:
  void Shutdown() override;

  // The BrowserList for which this object is providing BrowserWebStateHelpers.
  BrowserList* browser_list_;

  // Attaches the BrowserWebStateHelpers to |browser|.
  void AttachHelpers(Browser* browser);

  DISALLOW_COPY_AND_ASSIGN(BrowserWebStateHelperDataSourceImpl);
};

#endif  // IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_IMPL_H_

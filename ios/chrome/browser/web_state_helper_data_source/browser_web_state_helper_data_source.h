// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_H_
#define IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_H_

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"

class BrowserList;

// BrowserWebStateHelperDataSource provides BrowserWebStateHelpers to the
// Browsers added to a ChromeBrowserState.
class BrowserWebStateHelperDataSource : public KeyedService {
 public:
  BrowserWebStateHelperDataSource() = default;
  ~BrowserWebStateHelperDataSource() override = default;

  // Instructs the data source to provide helpers to the Browsers in
  // |browser_list|.
  virtual void ProvideHelpersToBrowsers(BrowserList* browser_list) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserWebStateHelperDataSource);
};

#endif  // IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_H_

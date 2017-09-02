// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_FACTORY_H_
#define IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}
namespace ios {
class ChromeBrowserState;
}
class BrowserWebStateHelperDataSource;

// BrowserWebStateHelperDataSourceFactory attaches
// BrowserWebStateHelperDataSources to ChromeBrowserStates.
class BrowserWebStateHelperDataSourceFactory
    : public BrowserStateKeyedServiceFactory {
 public:
  // Convenience getter that typecasts the value returned to OverlayService.
  static BrowserWebStateHelperDataSource* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  // Getter for singleton instance.
  static BrowserWebStateHelperDataSourceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      BrowserWebStateHelperDataSourceFactory>;

  BrowserWebStateHelperDataSourceFactory();
  ~BrowserWebStateHelperDataSourceFactory() override;

  // BrowserStateKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(BrowserWebStateHelperDataSourceFactory);
};

#endif  // IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_FACTORY_H_

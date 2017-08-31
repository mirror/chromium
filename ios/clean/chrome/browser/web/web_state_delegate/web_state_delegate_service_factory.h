// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_WEB_WEB_STATE_DELEGATE_WEB_STATE_DELEGATE_SERVICE_FACTORY_H_
#define IOS_CLEAN_CHROME_BROWSER_WEB_WEB_STATE_DELEGATE_WEB_STATE_DELEGATE_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace ios {
class ChromeBrowserState;
}

class WebStateDelegateService;

// WebStateDelegateServiceFactory attaches WebStateDelegateServices to
// ChromeBrowserStates.
class WebStateDelegateServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  // Convenience getter that typecasts the value returned to OverlayService.
  static WebStateDelegateService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  // Getter for singleton instance.
  static WebStateDelegateServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<WebStateDelegateServiceFactory>;

  WebStateDelegateServiceFactory();
  ~WebStateDelegateServiceFactory() override;

  // BrowserStateKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(WebStateDelegateServiceFactory);
};

#endif  // IOS_CLEAN_CHROME_BROWSER_WEB_WEB_STATE_DELEGATE_WEB_STATE_DELEGATE_SERVICE_FACTORY_H_

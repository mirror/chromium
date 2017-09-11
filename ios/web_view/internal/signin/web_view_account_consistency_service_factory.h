// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_ACCOUNT_CONSISTENCY_SERVICE_FACTORY_H_
#define IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_ACCOUNT_CONSISTENCY_SERVICE_FACTORY_H_

#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

class AccountConsistencyService;

namespace ios_web_view {

class WebViewBrowserState;
// Singleton that owns the AccountConsistencyService(s) and associates them with
// browser states.
class WebViewAccountConsistencyServiceFactory
    : public BrowserStateKeyedServiceFactory {
 public:
  // Returns the instance of AccountConsistencyService associated with this
  // browser state (creating one if none exists). Returns null if this browser
  // state cannot have an AccountConsistencyService (for example, if it is
  // incognito or if WKWebView is not enabled).
  static AccountConsistencyService* GetForBrowserState(
      WebViewBrowserState* browser_state);

  // Returns an instance of the factory singleton.
  static WebViewAccountConsistencyServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      WebViewAccountConsistencyServiceFactory>;

  WebViewAccountConsistencyServiceFactory();
  ~WebViewAccountConsistencyServiceFactory() override;

  // BrowserStateKeyedServiceFactory:
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(WebViewAccountConsistencyServiceFactory);
};

}  // namespace ios_web_view

#endif  // IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_ACCOUNT_CONSISTENCY_SERVICE_FACTORY_H_

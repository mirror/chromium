// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_ACCOUNT_RECONCILOR_FACTORY_H_
#define IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_ACCOUNT_RECONCILOR_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

class AccountReconcilor;

namespace ios_web_view {

class WebViewBrowserState;

// Singleton that owns all AccountReconcilors and associates them with browser
// states.
class WebViewAccountReconcilorFactory : public BrowserStateKeyedServiceFactory {
 public:
  // Returns the instance of AccountReconcilor associated with this browser
  // state (creating one if none exists). Returns null if this browser state
  // cannot have an GaiaCookieManagerService (for example, if it is incognito).
  static AccountReconcilor* GetForBrowserState(
      ios_web_view::WebViewBrowserState* browser_state);

  // Returns an instance of the factory singleton.
  static WebViewAccountReconcilorFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<WebViewAccountReconcilorFactory>;

  WebViewAccountReconcilorFactory();
  ~WebViewAccountReconcilorFactory() override;

  // BrowserStateKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(WebViewAccountReconcilorFactory);
};

}  // namespace ios_web_view

#endif  // IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_ACCOUNT_RECONCILOR_FACTORY_H_

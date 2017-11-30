// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_AUTOFILL_ADDRESS_NORMALIZER_FACTORY_H_
#define IOS_CHROME_BROWSER_AUTOFILL_ADDRESS_NORMALIZER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace ios {
class ChromeBrowserState;
}  // namespace ios

namespace web {
class BrowserState;
}  // namespace web

namespace autofill {
class AddressNormalizer;

// Ensures that there's only one instance of autofill::AddressNormalizer per
// browser state.
class AddressNormalizerFactory : public BrowserStateKeyedServiceFactory {
 public:
  static AddressNormalizer* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static AddressNormalizerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<AddressNormalizerFactory>;

  AddressNormalizerFactory();
  ~AddressNormalizerFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(AddressNormalizerFactory);
};

}  // namespace autofill

#endif  // IOS_CHROME_BROWSER_AUTOFILL_ADDRESS_NORMALIZER_FACTORY_H_

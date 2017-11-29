// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/autofill/address_normalizer_factory.h"

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "components/autofill/core/browser/address_normalizer_impl.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/autofill/validation_rules_storage_factory.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/web/public/browser_state.h"
#include "third_party/libaddressinput/chromium/chrome_metadata_source.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/source.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/storage.h"

namespace autofill {
namespace {

std::unique_ptr<::i18n::addressinput::Source> GetAddressInputSource(
    net::URLRequestContextGetter* url_context_getter) {
  return std::unique_ptr<::i18n::addressinput::Source>(
      new autofill::ChromeMetadataSource(I18N_ADDRESS_VALIDATION_DATA_URL,
                                         url_context_getter));
}

std::unique_ptr<::i18n::addressinput::Storage> GetAddressInputStorage() {
  return autofill::cat ::CreateStorage();
}

}  // namespace

// static
autofill::AddressNormalizer* AddressNormalizerFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<autofill::AddressNormalizer*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
AddressNormalizerFactory* AddressNormalizerFactory::GetInstance() {
  return base::Singleton<AddressNormalizerFactory>::get();
}

AddressNormalizerFactory::AddressNormalizerFactory()
    : BrowserStateKeyedServiceFactory(
          "AddressNormalizer",
          BrowserStateDependencyManager::GetInstance()) {}

AddressNormalizerFactory::~AddressNormalizerFactory() {}

std::unique_ptr<KeyedService> AddressNormalizerFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return base::WrapUnique(new autofill::AddressNormalizerImpl(
      GetAddressInputSource(
          GetApplicationContext()->GetSystemURLRequestContext()),
      GetAddressInputStorage(),
      GetApplicationContext()->GetApplicationLocale()));
}

web::BrowserState* AddressNormalizerFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace autofill

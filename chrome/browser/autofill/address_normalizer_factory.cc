// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autofill/address_normalizer_factory.h"

#include <memory>
#include <utility>

#include "chrome/browser/autofill/validation_rules_storage_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/autofill/core/browser/address_normalizer_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"
#include "third_party/libaddressinput/chromium/chrome_metadata_source.h"
#include "third_party/libaddressinput/chromium/chrome_storage_impl.h"

namespace autofill {

// static
AddressNormalizerImpl* AddressNormalizerFactory::GetForProfile(
    Profile* profile) {
  return static_cast<AddressNormalizerImpl*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AddressNormalizerFactory* AddressNormalizerFactory::GetInstance() {
  return base::Singleton<AddressNormalizerFactory>::get();
}

AddressNormalizerFactory::AddressNormalizerFactory()
    : BrowserContextKeyedServiceFactory(
          "AddressNormalizerImpl",
          BrowserContextDependencyManager::GetInstance()) {}

AddressNormalizerFactory::~AddressNormalizerFactory() {}

KeyedService* AddressNormalizerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new AddressNormalizerImpl(
      std::unique_ptr<::i18n::addressinput::Source>(
          new autofill::ChromeMetadataSource(
              I18N_ADDRESS_VALIDATION_DATA_URL,
              static_cast<Profile*>(context)->GetRequestContext())),
      ValidationRulesStorageFactory::CreateStorage());
}

}  // namespace autofill

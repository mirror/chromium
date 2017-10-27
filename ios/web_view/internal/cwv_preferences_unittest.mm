// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/cwv_preferences_internal.h"

#import <Foundation/Foundation.h>
#include <memory>

#include "base/memory/ptr_util.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/in_memory_pref_store.h"
#include "components/prefs/pref_service_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_web_view {

using CWVPreferencesTest = PlatformTest;

// Tests CWVPreferences |autofillEnabled|.
TEST_F(CWVPreferencesTest, Initialization) {
  auto pref_registry = new user_prefs::PrefRegistrySyncable();
  autofill::AutofillManager::RegisterProfilePrefs(pref_registry);
  auto pref_store = new InMemoryPrefStore();
  PrefServiceFactory factory;
  factory.set_user_prefs(pref_store);
  auto pref_service = factory.Create(pref_registry);
  CWVPreferences* preferences =
      [[CWVPreferences alloc] initWithPrefService:pref_service.get()];
  ASSERT_TRUE(preferences.autofillEnabled);
  preferences.autofillEnabled = NO;
  ASSERT_FALSE(preferences.autofillEnabled);
//  delete pref_registry;
//  delete pref_store;
}

}  // namespace ios_web_view

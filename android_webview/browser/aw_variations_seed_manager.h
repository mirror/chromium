// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_VARIATIONS_SEED_MANAGER_H_
#define ANDROID_WEBVIEW_BROWSER_AW_VARIATIONS_SEED_MANAGER_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "components/variations/service/ui_string_overrider.h"
#include "components/variations/service/variations_seed_manager.h"
#include "components/variations/service/variations_service_client.h"

// Used to setup field trials based on stored variations seed data
class AwVariationsSeedManager : public variations::VariationsSeedManager {
 public:
  AwVariationsSeedManager(
      PrefService* local_state,
      std::unique_ptr<variations::VariationsServiceClient> client,
      const variations::UIStringOverrider& ui_string_overrider);
  ~AwVariationsSeedManager();

  std::unique_ptr<const base::FieldTrial::EntropyProvider>
  CreateLowEntropyProvider() override;
};

#endif  // ANDROID_WEBVIEW_BROWSER_AW_VARIATIONS_SEED_MANAGER_H_

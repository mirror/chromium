// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_field_trial_creator.h"

#include <jni.h>

#include "android_webview/browser/aw_metrics_service_client.h"
#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/jni_weak_ref.h"
#include "base/base_switches.h"
#include "base/feature_list.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "cc/base/switches.h"
#include "components/prefs/in_memory_pref_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/variations/entropy_provider.h"
#include "components/variations/pref_names.h"
#include "content/public/common/content_switches.h"
#include "jni/AwVariationsDataHandler_jni.h"

using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF8;
using base::android::ScopedJavaLocalRef;

namespace android_webview {
namespace {

// TODO(kmilka): Update to work properly in environments both with and without
// UMA enabled.
std::unique_ptr<const base::FieldTrial::EntropyProvider>
CreateLowEntropyProvider() {
  return std::unique_ptr<const base::FieldTrial::EntropyProvider>(
      // Since variations are only enabled for users opted in to UMA, it is
      // acceptable to use the SHA1EntropyProvider for randomization.
      new metrics::SHA1EntropyProvider(
          // Synchronous read of the client id is permitted as it is fast
          // enough to have minimal impact on startup time, and is behind the
          // webview-enable-finch flag.
          android_webview::AwMetricsServiceClient::GetClientId()));
}

// Synchronous read of variations data is permitted as it is fast
// enough to have minimal impact on startup time, and is behind the
// webview-enable-finch flag.
bool ReadVariationsSeedDataFromFile(PrefService* local_state) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> j_variations_seed_data =
      Java_AwVariationsDataHandler_readVariationsSeedData(env);

  std::string data_str = ConvertJavaStringToUTF8(
      Java_VariationsSeedData_getData(env, j_variations_seed_data));

  // If the data string is empty an error was caught and logged in Java.
  if (data_str.empty()) {
    return false;
  }

  std::string signature_str = ConvertJavaStringToUTF8(
      Java_VariationsSeedData_getSignature(env, j_variations_seed_data));
  std::string country_str = ConvertJavaStringToUTF8(
      Java_VariationsSeedData_getCountry(env, j_variations_seed_data));
  int64_t fetch_time =
      Java_VariationsSeedData_getFetchTime(env, j_variations_seed_data);

  local_state->SetString(variations::prefs::kVariationsCompressedSeed,
                         data_str);
  local_state->SetString(variations::prefs::kVariationsSeedSignature,
                         signature_str);
  local_state->SetString(variations::prefs::kVariationsCountry, country_str);
  local_state->SetInt64(variations::prefs::kVariationsSeedDate, fetch_time);
  local_state->SetInt64(variations::prefs::kVariationsLastFetchTime,
                        fetch_time);

  return true;
}

}  // anonymous namespace

AwFieldTrialCreator::AwFieldTrialCreator()
    : aw_field_trials_(base::MakeUnique<AwFieldTrials>()) {}

AwFieldTrialCreator::~AwFieldTrialCreator() {}

std::unique_ptr<PrefService> AwFieldTrialCreator::CreateLocalState() {
  scoped_refptr<PrefRegistrySimple> pref_registry =
      base::MakeRefCounted<PrefRegistrySimple>();

  // Register the variations prefs with default values that must be overridden.
  pref_registry->RegisterInt64Pref(variations::prefs::kVariationsSeedDate, 0);
  pref_registry->RegisterInt64Pref(variations::prefs::kVariationsLastFetchTime,
                                   0);
  pref_registry->RegisterStringPref(variations::prefs::kVariationsCountry,
                                    std::string());
  pref_registry->RegisterStringPref(
      variations::prefs::kVariationsCompressedSeed, std::string());
  pref_registry->RegisterStringPref(variations::prefs::kVariationsSeedSignature,
                                    std::string());
  pref_registry->RegisterListPref(
      variations::prefs::kVariationsPermanentConsistencyCountry,
      base::MakeUnique<base::ListValue>());

  pref_service_factory_.set_user_prefs(
      make_scoped_refptr(new InMemoryPrefStore()));
  return pref_service_factory_.Create(pref_registry.get());
}

void AwFieldTrialCreator::SetUpFieldTrials() {
  if (!AwMetricsServiceClient::CheckSDKVersionForMetrics())
    return;

  AwMetricsServiceClient::LoadOrCreateClientId();

  DCHECK(!field_trial_list_);
  // Set the FieldTrialList singleton.
  field_trial_list_ =
      base::MakeUnique<base::FieldTrialList>(CreateLowEntropyProvider());

  std::unique_ptr<PrefService> local_state = CreateLocalState();

  if (!ReadVariationsSeedDataFromFile(local_state.get()))
    return;

  variations::UIStringOverrider ui_string_overrider;
  client_ = base::MakeUnique<AwVariationsServiceClient>();
  variations_field_trial_creator_ =
      base::MakeUnique<variations::VariationsFieldTrialCreator>(
          local_state.get(), client_.get(), ui_string_overrider);

  variations_field_trial_creator_->OverrideVariationsPlatform(
      variations::Study::PLATFORM_ANDROID_WEBVIEW);

  // Unused by WebView, but required by
  // VariationsFieldTrialCreator::SetupFieldTrials().
  std::vector<std::string> variation_ids;
  std::set<std::string> unforceable_field_trials;

  // Populates the FieldTrialList singleton via the static member functions.
  variations_field_trial_creator_->SetupFieldTrials(
      cc::switches::kEnableGpuBenchmarking, switches::kEnableFeatures,
      switches::kDisableFeatures, unforceable_field_trials,
      CreateLowEntropyProvider(), base::MakeUnique<base::FeatureList>(),
      &variation_ids, aw_field_trials_.get());
}

}  // namespace android_webview

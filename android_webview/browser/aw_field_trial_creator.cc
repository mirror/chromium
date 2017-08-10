// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_field_trial_creator.h"

#include "android_webview/browser/aw_metrics_service_client.h"
#include "android_webview/browser/aw_variations_service_client.h"
#include "base/base_switches.h"
#include "base/feature_list.h"
#include "base/strings/string_split.h"
#include "base/path_service.h"
#include "cc/base/switches.h"
#include "components/prefs/in_memory_pref_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/variations/entropy_provider.h"
#include "components/variations/pref_names.h"
#include "content/public/common/content_switches.h"

namespace android_webview {
namespace {

const std::string kVariationsSeedData = "variations_seed_data";
const std::string kVariationsSeedPref = "variations_seed_pref";

std::unique_ptr<const base::FieldTrial::EntropyProvider>
CreateLowEntropyProvider() {
  return std::unique_ptr<const base::FieldTrial::EntropyProvider>(
      // Since variations are only enabled for users opted in to UMA, it is
      // acceptable to use the SHA1EntropyProvider for randomization.
      new metrics::SHA1EntropyProvider(
          // Synchronous read of the client id is permitted as it is fast
          // enough to have minimal impact on startup time, and is behind the
          // webview-enable-finch flag.
          android_webview::AwMetricsServiceClient::GetOrCreateClientId()));
}

// Synchronous read of variations data is permitted as it is fast
// enough to have minimal impact on startup time, and is behind the
// webview-enable-finch flag.
bool ReadVariationsSeedDataFromFile(PrefService* local_state) {
  base::FilePath user_data_dir;
  if (!PathService::Get(base::DIR_ANDROID_APP_DATA, &user_data_dir)) {
    LOG(ERROR) << "Failed to get app data directory for Android WebView";
    return false;
  }

  const base::FilePath variations_seed_path =
      user_data_dir.Append(FILE_PATH_LITERAL(kVariationsSeedData));
  const base::FilePath variations_pref_path =
      user_data_dir.Append(FILE_PATH_LITERAL(kVariationsSeedPref));

  // Set compressed seed data.
  std::string seed_str;
  if (!base::ReadFileToString(variations_seed_path, &seed_str)) {
    LOG(ERROR) << "Failed to read variations seed data";
    return false;
  }

  // Set seed meta-data.
  std::string data_str;
  if (!base::ReadFileToString(variations_pref_path, &data_str)) {
    LOG(ERROR) << "Failed to read variations seed meta-data";
    return false;
  }

  std::vector<std::string> tokens = base::SplitString(
      data_str, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  if (tokens.size() != 6) {
    LOG(ERROR) << "variations_seed_pref contained wrong number of lines";
    return false;
  }

  if (tokens[1].length() != 2) {
    LOG(ERROR) << "Variations country code has invalid length";
    return false;
  }

  base::Time fetch_date;
  if (!base::Time::FromUTCString(tokens[2].c_str(), &fetch_date)) {
    LOG(ERROR) << "Failed to parse seed last fetch date from string";
    return false;
  }

  local_state->SetString(variations::prefs::kVariationsCompressedSeed,
                         seed_str);
  local_state->SetString(variations::prefs::kVariationsSeedSignature,
                         tokens[0]);
  local_state->SetString(variations::prefs::kVariationsCountry, tokens[1]);
  local_state->SetInt64(variations::prefs::kVariationsSeedDate,
                        fetch_date.ToInternalValue());
  local_state->SetInt64(variations::prefs::kVariationsLastFetchTime,
                        fetch_date.ToInternalValue());

  return true;
}

}  // anonymous namespace

AwFieldTrialCreator::AwFieldTrialCreator() {
}

AwFieldTrialCreator::~AwFieldTrialCreator() {
}

std::unique_ptr<PrefService> AwFieldTrialCreator::CreateLocalState() {
  PrefRegistrySimple* pref_registry =
      new PrefRegistrySimple();
  pref_registry->RegisterInt64Pref(variations::prefs::kVariationsSeedDate, 0);
  pref_registry->RegisterInt64Pref(variations::prefs::kVariationsLastFetchTime,
                                   0);
  pref_registry->RegisterStringPref(variations::prefs::kVariationsCountry,
                                    "");
  pref_registry->RegisterStringPref(
      variations::prefs::kVariationsCompressedSeed, "");
  pref_registry->RegisterStringPref(variations::prefs::kVariationsSeedSignature,
                                    "");
  std::unique_ptr<base::ListValue> default_value = base::MakeUnique<base::ListValue>();
  pref_registry->RegisterListPref(
      variations::prefs::kVariationsPermanentConsistencyCountry,
      std::move(default_value));

  pref_service_factory_.set_user_prefs(
      make_scoped_refptr(new InMemoryPrefStore()));
  return pref_service_factory_.Create(pref_registry);
}

void AwFieldTrialCreator::SetUpFieldTrials() {
  if (!AwMetricsServiceClient::CheckSDKVersionForMetrics())
    return;

  DCHECK(!field_trial_list_);
  field_trial_list_.reset(new base::FieldTrialList(CreateLowEntropyProvider()));

  std::unique_ptr<PrefService> local_state = CreateLocalState();

  if (!ReadVariationsSeedDataFromFile(local_state.get()))
    return;

  variations::UIStringOverrider ui_string_overrider;
  std::unique_ptr<AwVariationsServiceClient> client = base::MakeUnique<AwVariationsServiceClient>();
  variations_field_trial_creator_.reset(
      new variations::VariationsFieldTrialCreator(
          local_state.get(), client.get(), ui_string_overrider));

  variations_field_trial_creator_->OverrideVariationsPlatform(
      variations::Study::PLATFORM_ANDROID_WEBVIEW);

  std::unique_ptr<base::FeatureList> feature_list = base::MakeUnique<base::FeatureList>();
  std::vector<std::string> variation_ids;
  std::set<std::string> unforceable_field_trials;

  variations_field_trial_creator_->SetupFieldTrials(
      cc::switches::kEnableGpuBenchmarking, switches::kEnableFeatures,
      switches::kDisableFeatures, unforceable_field_trials, CreateLowEntropyProvider(),
      std::move(feature_list), &variation_ids, &webview_field_trials_);
}

}  // namespace android_webview

// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VARIATIONS_FIELD_TRIAL_CREATOR_H_
#define COMPONENTS_VARIATIONS_FIELD_TRIAL_CREATOR_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/threading/thread_checker.h"
#include "components/variations/client_filterable_state.h"
#include "components/variations/platform_field_trials.h"
#include "components/variations/service/ui_string_overrider.h"
#include "components/variations/service/variations_service_client.h"
#include "components/variations/variations_seed_store.h"

namespace variations {

// Used to setup field trials based on stored variations seed data
class VariationsFieldTrialCreator {
 public:
  VariationsFieldTrialCreator(PrefService* local_state,
                              std::unique_ptr<VariationsServiceClient> client,
                              const UIStringOverrider& ui_string_overrider);
  ~VariationsFieldTrialCreator();

  // Returns what variations will consider to be the latest country. Returns
  // empty if it is not available.
  std::string GetLatestCountry() const;

  // Creates field trials based on the variations seed loaded from local state.
  // If there is a problem loading the seed data, all trials specified by the
  // seed may not be created. Some field trials are configured to override or
  // associate with (for reporting) specific features. These associations are
  // registered with |feature_list|.
  bool CreateTrialsFromSeed(base::FeatureList* feature_list);

  // Create an entropy provider based on low entropy. This is used to create
  // trials for studies that should only depend on low entropy, such as studies
  // that send experiment IDs to Google web properties. Pure virtual so
  // Chrome and WebView must provide their own implementations.
  virtual std::unique_ptr<const base::FieldTrial::EntropyProvider>
  CreateLowEntropyProvider() = 0;

  bool SetupFieldTrials(std::unique_ptr<base::FieldTrialList>& field_trial_list,
                        std::unique_ptr<base::FeatureList>& feature_list,
                        std::vector<std::string>& variation_ids,
                        variations::PlatformFieldTrials* platform_field_trials);

 protected:
  // Returns all of the client state used for filtering studies.
  // As a side-effect, may update the stored permanent consistency country.
  std::unique_ptr<ClientFilterableState> GetClientFilterableStateForVersion(
      const base::Version& version);

  virtual bool LoadSeed(VariationsSeed* seed);

  // Loads the country code to use for filtering permanent consistency studies,
  // updating the stored country code if the stored value was for a different
  // Chrome version. The country used for permanent consistency studies is kept
  // consistent between Chrome upgrades in order to avoid annoying the user due
  // to experiment churn while traveling.
  std::string LoadPermanentConsistencyCountry(
      const base::Version& version,
      const std::string& latest_country);

  // Sets the stored permanent country pref for this client.
  void StorePermanentCountry(const base::Version& version,
                             const std::string& country);

  // Record the time of the most recent successful fetch.
  void RecordLastFetchTime();

  std::unique_ptr<VariationsServiceClient> client_;

  UIStringOverrider ui_string_overrider_;

  // The pref service used to store persist the variations seed.
  PrefService* local_state_;

  VariationsSeedStore seed_store_;

  // Tracks whether |CreateTrialsFromSeed| has been called, to ensure that
  // it gets called prior to |StartRepeatedVariationsSeedFetch|.
  bool create_trials_from_seed_called_;

  base::ThreadChecker thread_checker_;

  // Set of different possible values to report for the
  // Variations.LoadPermanentConsistencyCountryResult histogram. This enum must
  // be kept consistent with its counterpart in histograms.xml.
  enum LoadPermanentConsistencyCountryResult {
    LOAD_COUNTRY_NO_PREF_NO_SEED = 0,
    LOAD_COUNTRY_NO_PREF_HAS_SEED,
    LOAD_COUNTRY_INVALID_PREF_NO_SEED,
    LOAD_COUNTRY_INVALID_PREF_HAS_SEED,
    LOAD_COUNTRY_HAS_PREF_NO_SEED_VERSION_EQ,
    LOAD_COUNTRY_HAS_PREF_NO_SEED_VERSION_NEQ,
    LOAD_COUNTRY_HAS_BOTH_VERSION_EQ_COUNTRY_EQ,
    LOAD_COUNTRY_HAS_BOTH_VERSION_EQ_COUNTRY_NEQ,
    LOAD_COUNTRY_HAS_BOTH_VERSION_NEQ_COUNTRY_EQ,
    LOAD_COUNTRY_HAS_BOTH_VERSION_NEQ_COUNTRY_NEQ,
    LOAD_COUNTRY_MAX,
  };

 private:
  DISALLOW_COPY_AND_ASSIGN(VariationsFieldTrialCreator);
};

}  // namespace variations

#endif  // COMPONENTS_VARIATIONS_FIELD_TRIAL_CREATOR_H_

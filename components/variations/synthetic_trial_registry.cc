// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/synthetic_trial_registry.h"

namespace variations {

SyntheticTrialRegistry::SyntheticTrialRegistry() = default;
SyntheticTrialRegistry::~SyntheticTrialRegistry() = default;

void SyntheticTrialRegistry::AddSyntheticTrialObserver(
    SyntheticTrialObserver* observer) {
  synthetic_trial_observer_list_.AddObserver(observer);
  if (!synthetic_trial_groups_.empty())
    observer->OnSyntheticTrialsChanged(synthetic_trial_groups_);
}

void SyntheticTrialRegistry::RemoveSyntheticTrialObserver(
    SyntheticTrialObserver* observer) {
  synthetic_trial_observer_list_.RemoveObserver(observer);
}

void SyntheticTrialRegistry::RegisterSyntheticFieldTrial(
    const SyntheticTrialGroup& trial) {
  for (size_t i = 0; i < synthetic_trial_groups_.size(); ++i) {
    if (synthetic_trial_groups_[i].id.name == trial.id.name) {
      if (synthetic_trial_groups_[i].id.group != trial.id.group) {
        synthetic_trial_groups_[i].id.group = trial.id.group;
        synthetic_trial_groups_[i].start_time = base::TimeTicks::Now();
        NotifySyntheticTrialObservers();
      }
      return;
    }
  }

  SyntheticTrialGroup trial_group = trial;
  trial_group.start_time = base::TimeTicks::Now();
  synthetic_trial_groups_.push_back(trial_group);
  NotifySyntheticTrialObservers();
}

void SyntheticTrialRegistry::NotifySyntheticTrialObservers() {
  for (SyntheticTrialObserver& observer : synthetic_trial_observer_list_) {
    observer.OnSyntheticTrialsChanged(synthetic_trial_groups_);
  }
}

void SyntheticTrialRegistry::GetSyntheticFieldTrialsOlderThan(
    base::TimeTicks time,
    std::vector<ActiveGroupId>* synthetic_trials) {
  DCHECK(synthetic_trials);
  synthetic_trials->clear();
  for (size_t i = 0; i < synthetic_trial_groups_.size(); ++i) {
    if (synthetic_trial_groups_[i].start_time <= time)
      synthetic_trials->push_back(synthetic_trial_groups_[i].id);
  }
}

}  // namespace variations

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement_tracker/test/test_feature_engagement_tracker.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/feature_engagement_tracker/internal/availability_model_impl.h"
#include "components/feature_engagement_tracker/internal/chrome_variations_configuration.h"
#include "components/feature_engagement_tracker/internal/feature_config_condition_validator.h"
#include "components/feature_engagement_tracker/internal/feature_config_storage_validator.h"
#include "components/feature_engagement_tracker/internal/feature_engagement_tracker_impl.h"
#include "components/feature_engagement_tracker/internal/in_memory_store.h"
#include "components/feature_engagement_tracker/internal/init_aware_model.h"
#include "components/feature_engagement_tracker/internal/model_impl.h"
#include "components/feature_engagement_tracker/internal/storage_validator.h"
#include "components/feature_engagement_tracker/internal/store.h"
#include "components/feature_engagement_tracker/internal/system_time_provider.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"
#include "components/feature_engagement_tracker/public/feature_list.h"

namespace feature_engagement_tracker {

namespace {

class TestAvailabilityModel : public AvailabilityModel {
 public:
  TestAvailabilityModel() = default;
  ~TestAvailabilityModel() override = default;

  void Initialize(AvailabilityModel::OnInitializedCallback callback,
                  uint32_t current_day) override {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), true));
  }

  bool IsReady() const override { return true; }

  base::Optional<uint32_t> GetAvailability(
      const base::Feature& feature) const override {
    auto search = availabilities_.find(feature.name);
    if (search == availabilities_.end())
      return base::nullopt;

    return search->second;
  }

  void SetAvailability(const base::Feature* feature,
                       base::Optional<uint32_t> availability) {
    availabilities_[feature->name] = availability;
  }

 private:
  std::map<std::string, base::Optional<uint32_t>> availabilities_;

  DISALLOW_COPY_AND_ASSIGN(TestAvailabilityModel);
};

class TestInMemoryStore : public InMemoryStore {
 public:
  explicit TestInMemoryStore(std::unique_ptr<std::vector<Event>> events)
      : events_(std::move(events)) {}

  void Load(const OnLoadedCallback& callback) override {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(callback, true, base::Passed(&events_)));
  }

 private:
  std::unique_ptr<std::vector<Event>> events_;

  DISALLOW_COPY_AND_ASSIGN(TestInMemoryStore);
};

class StoreEverythingStorageValidator : public StorageValidator {
 public:
  StoreEverythingStorageValidator() = default;
  ~StoreEverythingStorageValidator() override = default;

  bool ShouldStore(const std::string& event_name) const override {
    return true;
  }

  bool ShouldKeep(const std::string& event_name,
                  uint32_t event_day,
                  uint32_t current_day) const override {
    return true;
  };

 private:
  DISALLOW_COPY_AND_ASSIGN(StoreEverythingStorageValidator);
};

}  // namespace

// static
std::unique_ptr<FeatureEngagementTracker> CreateTestFeatureEngagementTracker() {
  auto configuration = base::MakeUnique<ChromeVariationsConfiguration>();
  configuration->ParseFeatureConfigs(GetAllFeatures());

  auto store = base::MakeUnique<TestInMemoryStore>(
      base::MakeUnique<std::vector<Event>>());

  auto storage_validator = base::MakeUnique<FeatureConfigStorageValidator>();
  storage_validator->InitializeFeatures(GetAllFeatures(), *configuration);

  auto raw_model = base::MakeUnique<ModelImpl>(std::move(store),
                                               std::move(storage_validator));

  auto model = base::MakeUnique<InitAwareModel>(std::move(raw_model));

  auto availability_model = base::MakeUnique<TestAvailabilityModel>();

  return base::MakeUnique<FeatureEngagementTrackerImpl>(
      std::move(model), std::move(availability_model), std::move(configuration),
      base::MakeUnique<FeatureConfigConditionValidator>(),
      base::MakeUnique<SystemTimeProvider>());
}

}  // namespace feature_engagement_tracker

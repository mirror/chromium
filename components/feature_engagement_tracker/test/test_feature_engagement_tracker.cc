// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement_tracker/test/test_feature_engagement_tracker.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "components/feature_engagement_tracker/internal/availability_model_impl.h"
#include "components/feature_engagement_tracker/internal/chrome_variations_configuration.h"
#include "components/feature_engagement_tracker/internal/feature_config_condition_validator.h"
#include "components/feature_engagement_tracker/internal/feature_config_storage_validator.h"
#include "components/feature_engagement_tracker/internal/feature_engagement_tracker_impl.h"
#include "components/feature_engagement_tracker/internal/in_memory_store.h"
#include "components/feature_engagement_tracker/internal/model_impl.h"
#include "components/feature_engagement_tracker/internal/storage_validator.h"
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
    return base::nullopt;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestAvailabilityModel);
};

// An InMemoryStore that is able to fake successful and unsuccessful
// loading of state.
class TestInMemoryStore : public InMemoryStore {
 public:
  explicit TestInMemoryStore() : InMemoryStore() {}

  void Load(const OnLoadedCallback& callback) override {
    HandleLoadResult(callback, true);
  }

 private:
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
  std::vector<const base::Feature*> features = {&kIPHNewTabFeature};

  auto store = base::MakeUnique<TestInMemoryStore>();

  auto model = base::MakeUnique<ModelImpl>(
      std::move(store), base::MakeUnique<StoreEverythingStorageValidator>());

  auto availability_model = base::MakeUnique<TestAvailabilityModel>();

  auto configuration = base::MakeUnique<ChromeVariationsConfiguration>();
  configuration->ParseFeatureConfigs(features);

  return base::MakeUnique<FeatureEngagementTrackerImpl>(
      std::move(model), std::move(availability_model), std::move(configuration),
      base::MakeUnique<FeatureConfigConditionValidator>(),
      base::MakeUnique<SystemTimeProvider>());
}

}  // namespace feature_engagement_tracker

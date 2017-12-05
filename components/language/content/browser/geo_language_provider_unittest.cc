// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/content/browser/geo_language_provider.h"

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/timer/timer.h"
#include "device/geolocation/public/interfaces/geolocation.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gtest/include/gtest/gtest.h"

class MockGeoLocation : public device::mojom::Geolocation {
 public:
  explicit MockGeoLocation(device::mojom::GeolocationRequest request)
      : binding_(this, std::move(request)) {}

  // device::mojom::Geolocation implementation:
  void SetHighAccuracy(bool high_accuracy) override {}
  void QueryNextPosition(QueryNextPositionCallback callback) override {
    ++query_next_position_called_times_;
    device::mojom::Geoposition position;
    // Setup a random place in Madhya Pradesh, India.
    position.latitude = 23.0;
    position.longitude = 80.0;
    std::move(callback).Run(position.Clone());
  }

  int query_next_position_called_times() const {
    return query_next_position_called_times_;
  }

 private:
  int query_next_position_called_times_ = 0;
  mojo::Binding<device::mojom::Geolocation> binding_;
};

namespace language {

class GeoLanguageProviderTest : public testing::Test {
 public:
  GeoLanguageProviderTest()
      : task_runner_(base::MakeRefCounted<base::TestMockTimeTaskRunner>(
            base::TestMockTimeTaskRunner::Type::kBoundToThread)),
        scoped_context_(task_runner_.get()),
        geo_language_provider_(task_runner_, task_runner_),
        mock_geo_location_(mojo::MakeRequest(
            &geo_language_provider_.ip_geolocation_service_)) {}

 protected:
  std::vector<std::string> GetCurrentGeoLanguages() {
    return geo_language_provider_.CurrentGeoLanguages();
  }

  void StartGeoLanguageProvider() {
    geo_language_provider_.BackgroundStartUp();
  }

  const scoped_refptr<base::TestMockTimeTaskRunner>& GetTaskRunner() {
    return task_runner_;
  }

  int GetQueryNextPositionCalledTimes() {
    return mock_geo_location_.query_next_position_called_times();
  }

 private:
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  const base::TestMockTimeTaskRunner::ScopedContext scoped_context_;

  // Object under test.
  GeoLanguageProvider geo_language_provider_;
  MockGeoLocation mock_geo_location_;
};

TEST_F(GeoLanguageProviderTest, GetCurrentGeoLanguages) {
  StartGeoLanguageProvider();
  const auto task_runner = GetTaskRunner();
  task_runner->RunUntilIdle();

  const std::vector<std::string>& result = GetCurrentGeoLanguages();
  std::vector<std::string> expected_langs = {"hi", "mr", "ur"};
  EXPECT_EQ(expected_langs, result);
  EXPECT_EQ(1, GetQueryNextPositionCalledTimes());
}

TEST_F(GeoLanguageProviderTest, NoFrequentCalls) {
  StartGeoLanguageProvider();
  const auto task_runner = GetTaskRunner();
  task_runner->RunUntilIdle();

  const std::vector<std::string>& result = GetCurrentGeoLanguages();
  std::vector<std::string> expected_langs = {"hi", "mr", "ur"};
  EXPECT_EQ(expected_langs, result);

  task_runner->FastForwardBy(base::TimeDelta::FromHours(12));
  EXPECT_EQ(1, GetQueryNextPositionCalledTimes());
}

TEST_F(GeoLanguageProviderTest, ButDoCallInTheNextDay) {
  StartGeoLanguageProvider();
  const auto task_runner = GetTaskRunner();
  task_runner->RunUntilIdle();

  const std::vector<std::string>& result = GetCurrentGeoLanguages();
  std::vector<std::string> expected_langs = {"hi", "mr", "ur"};
  EXPECT_EQ(expected_langs, result);

  task_runner->FastForwardBy(base::TimeDelta::FromHours(25));
  EXPECT_EQ(2, GetQueryNextPositionCalledTimes());
}

}  // namespace language
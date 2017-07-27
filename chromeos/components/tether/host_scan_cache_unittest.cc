// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/host_scan_cache.h"

#include <unordered_map>

#include "base/memory/ptr_util.h"
#include "chromeos/components/tether/fake_host_scan_cache.h"
#include "chromeos/components/tether/host_scan_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace tether {

class TestObserver : public HostScanCache::Observer {
 public:
  TestObserver() : empty_cache_count_(0) {}

  int empty_cache_count() const { return empty_cache_count_; }

  void OnCacheBecameEmpty() override { empty_cache_count_++; }

 private:
  int empty_cache_count_;
};

class HostScanCacheTest : public testing::Test {
 protected:
  HostScanCacheTest()
      : test_entries_(host_scan_test_util::CreateTestEntries()) {}

  void SetUp() override {
    host_scan_cache_ = base::MakeUnique<FakeHostScanCache>();
  }

  const std::unordered_map<std::string, HostScanCacheEntry> test_entries_;

  std::unique_ptr<FakeHostScanCache> host_scan_cache_;
};

TEST_F(HostScanCacheTest, TestSetAndRemove) {
  TestObserver* observer = new TestObserver();

  host_scan_cache_->AddObserver(observer);

  host_scan_cache_->SetHostScanResult(
      test_entries_.at(host_scan_test_util::kTetherGuid0));
  EXPECT_EQ(1u, host_scan_cache_->size());

  host_scan_cache_->RemoveHostScanResult(host_scan_test_util::kTetherGuid0);
  EXPECT_EQ(0u, host_scan_cache_->size());
  EXPECT_EQ(1, observer->empty_cache_count());

  host_scan_cache_->RemoveObserver(observer);
}

}  // namespace tether

}  // namespace chromeos

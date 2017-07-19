// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_CLIENT_HIT_TEST_DATA_PROVIDER_H_
#define COMPONENTS_VIZ_CLIENT_HIT_TEST_DATA_PROVIDER_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "components/viz/common/hit_test/aggregated_hit_test_region.h"

namespace viz {

class HitTestDataProvider {
 public:
  using HitTestRegionList = std::vector<AggregatedHitTestRegion>;

  HitTestDataProvider() = default;
  virtual ~HitTestDataProvider() = default;

  // Returns an array of hit-test regions.
  virtual std::unique_ptr<HitTestRegionList> GetHitTestData() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(HitTestDataProvider);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_CLIENT_HIT_TEST_DATA_PROVIDER_H_

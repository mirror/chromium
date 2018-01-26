// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_HIT_TEST_HIT_TEST_REGION_LIST_BUILDER_H_
#define COMPONENTS_VIZ_COMMON_HIT_TEST_HIT_TEST_REGION_LIST_BUILDER_H_

#include "components/viz/common/quads/compositor_frame.h"
#include "services/viz/public/interfaces/hit_test/hit_test_region_list.mojom.h"

namespace viz {

class HitTestRegionListBuilder {
 public:
  static mojom::HitTestRegionListPtr CreateHitTestData(
      const CompositorFrame& frame);

 private:
  DISALLOW_COPY_AND_ASSIGN(HitTestRegionListBuilder);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_HIT_TEST_HIT_TEST_REGION_LIST_BUILDER_H_

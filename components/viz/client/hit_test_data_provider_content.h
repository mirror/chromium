// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_CLIENT_HIT_TEST_DATA_PROVIDER_CONTENT_H_
#define COMPONENTS_VIZ_CLIENT_HIT_TEST_DATA_PROVIDER_CONTENT_H_

#include "base/macros.h"
#include "components/viz/client/hit_test_data_provider.h"
#include "components/viz/client/viz_client_export.h"

namespace cc {

class LayerTreeHostImpl;
}

namespace viz {

class VIZ_CLIENT_EXPORT HitTestDataProviderContent
    : public HitTestDataProvider {
 public:
  explicit HitTestDataProviderContent(const cc::LayerTreeHostImpl* host_impl);
  ~HitTestDataProviderContent() override;

  mojom::HitTestRegionListPtr GetHitTestData() const override;

 private:
  void GetHitTestDataRecursively(
      const cc::LayerTreeHostImpl* host_impl,
      mojom::HitTestRegionList* hit_test_region_list) const;

  const cc::LayerTreeHostImpl* host_impl_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(HitTestDataProviderContent);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_CLIENT_HIT_TEST_DATA_PROVIDER_CONTENT_H_

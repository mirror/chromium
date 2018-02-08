// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_CLIENT_HIT_TEST_DATA_PROVIDER_DRAW_QUAD_RENDERER_H_
#define COMPONENTS_VIZ_CLIENT_HIT_TEST_DATA_PROVIDER_DRAW_QUAD_RENDERER_H_

#include "components/viz/client/hit_test_data_provider.h"

namespace viz {

// HitTestDataProviderDrawQuadRenderer is used to derive hit test data from
// the bounds data in the CompositorFrame and from DrawQuad in its
// RenderPassList.
class VIZ_CLIENT_EXPORT HitTestDataProviderDrawQuadRenderer
    : public HitTestDataProvider {
 public:
  HitTestDataProviderDrawQuadRenderer();
  ~HitTestDataProviderDrawQuadRenderer() override;

  mojom::HitTestRegionListPtr GetHitTestData(
      const CompositorFrame& compositor_frame) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(HitTestDataProviderDrawQuadRenderer);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_CLIENT_HIT_TEST_DATA_PROVIDER_DRAW_QUAD_RENDERER_H_

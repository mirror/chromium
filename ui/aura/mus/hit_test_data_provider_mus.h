// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_HIT_TEST_DATA_PROVIDER_MUS_H_
#define UI_AURA_MUS_HIT_TEST_DATA_PROVIDER_MUS_H_

#include "base/macros.h"
#include "components/viz/client/hit_test_data_provider.h"

namespace aura {

class Window;

class HitTestDataProviderMus : public viz::HitTestDataProvider {
 public:
  HitTestDataProviderMus(Window* window);
  ~HitTestDataProviderMus() override;

  // HitTestDataProvider:
  std::unique_ptr<HitTestRegionList> GetHitTestData() override;

 private:
  // Recursively walks the children of |window| and uses |window|'s
  // EventTargeter to generate hit-test data for the |window|'s descendants.
  // Populates |hit_test_region_list|.
  void GetHitTestDataRecursively(aura::Window* window,
                                 HitTestRegionList* hit_test_region_list);

  aura::Window* window_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(HitTestDataProviderMus);
};

}  // namespace aura

#endif  // UI_AURA_MUS_HIT_TEST_DATA_PROVIDER_MUS_H_
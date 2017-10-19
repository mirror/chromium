// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/interfaces/window_manager_struct_traits.h"

#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ui {
namespace {

// Tests the multi-display mode enum.
TEST(WindowManagerStructTraitsTest, MultiDisplayMode) {
  using MultiDisplayModeTraits = mojo::EnumTraits<ui::mojom::MultiDisplayMode, display::DisplayManager::MultiDisplayMode>;
  
  for (display::DisplayManager::MultiDisplayMode value : { display::DisplayManager::EXTENDED, display::DisplayManager::MIRRORING, display::DisplayManager::UNIFIED }) {
    display::DisplayManager::MultiDisplayMode echoed_value;
    mojom::MultiDisplayMode mojo_value = MultiDisplayModeTraits::ToMojom(value);
    ASSERT_TRUE(MultiDisplayModeTraits::FromMojom(mojo_value, &echoed_value));
    EXPECT_EQ(echoed_value, value);
  }
}

}  // namespace
}  // namespace ui

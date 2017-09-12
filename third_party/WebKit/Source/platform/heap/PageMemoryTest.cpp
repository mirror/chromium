// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/heap/PageMemory.h"

#include "platform/heap/VisitorImpl.h"
#include "platform/wtf/PtrUtil.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(PageMemoryTest, RegionTree) {
  std::unique_ptr<RegionTree> region_tree(WTF::MakeUnique<RegionTree>());
  std::unique_ptr<MemoryRegion> region180 =
      WTF::MakeUnique<MemoryRegion>(reinterpret_cast<Address>(180u), 10u);
  std::unique_ptr<MemoryRegion> region200 =
      WTF::MakeUnique<MemoryRegion>(reinterpret_cast<Address>(200u), 10u);
  std::unique_ptr<MemoryRegion> region210 =
      WTF::MakeUnique<MemoryRegion>(reinterpret_cast<Address>(210u), 10u);
  std::unique_ptr<MemoryRegion> region240 =
      WTF::MakeUnique<MemoryRegion>(reinterpret_cast<Address>(240u), 10u);
  std::unique_ptr<MemoryRegion> region260 =
      WTF::MakeUnique<MemoryRegion>(reinterpret_cast<Address>(260u), 10u);
  std::unique_ptr<MemoryRegion> region280 =
      WTF::MakeUnique<MemoryRegion>(reinterpret_cast<Address>(280u), 10u);

  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(200u)), nullptr);
  region_tree->Add(region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(199u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(200u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(209u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(210u)), nullptr);
  region_tree->Add(region210.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(199u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(200u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(209u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(210u)),
            region210.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(219u)),
            region210.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(220u)), nullptr);
  region_tree->Add(region180.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(179u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(180u)),
            region180.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(189u)),
            region180.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(190u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(199u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(200u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(209u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(210u)),
            region210.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(219u)),
            region210.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(220u)), nullptr);
  region_tree->Add(region240.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(179u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(180u)),
            region180.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(189u)),
            region180.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(290u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(199u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(200u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(209u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(210u)),
            region210.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(219u)),
            region210.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(220u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(239u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(240u)),
            region240.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(249u)),
            region240.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(250u)), nullptr);
  region_tree->Add(region260.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(179u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(180u)),
            region180.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(189u)),
            region180.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(290u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(199u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(200u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(209u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(210u)),
            region210.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(219u)),
            region210.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(220u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(239u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(240u)),
            region240.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(249u)),
            region240.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(250u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(259u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(260u)),
            region260.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(269u)),
            region260.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(270u)), nullptr);
  region_tree->Add(region280.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(179u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(180u)),
            region180.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(189u)),
            region180.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(290u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(199u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(200u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(209u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(210u)),
            region210.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(219u)),
            region210.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(220u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(239u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(240u)),
            region240.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(249u)),
            region240.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(250u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(259u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(260u)),
            region260.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(269u)),
            region260.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(270u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(279u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(280u)),
            region280.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(289u)),
            region280.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(290u)), nullptr);
  region_tree->Remove(region210.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(179u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(180u)),
            region180.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(189u)),
            region180.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(290u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(199u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(200u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(209u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(210u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(219u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(239u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(240u)),
            region240.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(249u)),
            region240.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(250u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(259u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(260u)),
            region260.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(269u)),
            region260.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(270u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(279u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(280u)),
            region280.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(289u)),
            region280.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(290u)), nullptr);
  region_tree->Remove(region240.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(179u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(180u)),
            region180.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(189u)),
            region180.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(290u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(199u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(200u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(209u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(210u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(239u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(240u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(249u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(250u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(259u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(260u)),
            region260.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(269u)),
            region260.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(270u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(279u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(280u)),
            region280.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(289u)),
            region280.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(290u)), nullptr);
  region_tree->Remove(region280.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(179u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(180u)),
            region180.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(189u)),
            region180.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(290u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(199u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(200u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(209u)),
            region200.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(210u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(259u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(260u)),
            region260.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(269u)),
            region260.get());
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(270u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(279u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(280u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(289u)), nullptr);
  EXPECT_EQ(region_tree->Lookup(reinterpret_cast<Address>(290u)), nullptr);
}

}  // namespace blink

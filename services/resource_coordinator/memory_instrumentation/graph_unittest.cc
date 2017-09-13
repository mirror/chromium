// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/memory_instrumentation/graph.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace memory_instrumentation {
TEST(GlobalDumpTest, CreateProcessDump) {
  GlobalDump global_dump;

  auto* dump = global_dump.CreateProcessDump(10);
  ASSERT_TRUE(dump != nullptr);

  auto& dumps = global_dump.dumps_for_testing();
  ASSERT_EQ(dump, dumps.find(10)->second.get());
}

TEST(DumpNodeTest, GetOrCreateChild) {
  auto global = std::make_unique<GlobalDump>();
  DumpNode node(global.get(), nullptr);

  auto* child = node.GetOrCreateChild("test");
  ASSERT_NE(child, nullptr);

  auto* act_child = node.GetChild("test");
  ASSERT_EQ(child, act_child);

  auto* none = node.GetChild("no child of this name");
  ASSERT_EQ(none, nullptr);
}

TEST(DumpNodeTest, AddEntry) {
  auto global = std::make_unique<GlobalDump>();
  DumpNode node(global.get(), nullptr);

  node.AddEntry("test", DumpNode::Entry::Units::kBytes, 100ul);
  auto entry = node.entries().find("test");
  ASSERT_EQ(entry->first, "test");
  ASSERT_EQ(entry->second.units, DumpNode::Entry::Units::kBytes);
  ASSERT_EQ(entry->second.value_uint64, 100ul);
}

}  // namespace memory_instrumentation
// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/memory_instrumentation/graph.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace memory_instrumentation {

TEST(GlobalDumpTest, CreateProcessDump) {
  GlobalDump global_dump;

  ContainerDump* dump = global_dump.CreateContainerForProcess(10);
  ASSERT_TRUE(dump != nullptr);

  auto& dumps = global_dump.container_dumps();
  ASSERT_EQ(dump, dumps.find(10)->second.get());
}

TEST(DumpNodeTest, GetChild) {
  auto global = std::make_unique<GlobalDump>();
  DumpNode node(global->global_container());

  ASSERT_EQ(node.GetChild("test"), nullptr);

  DumpNode child(global->global_container());
  node.PushChild("child", &child);
  ASSERT_EQ(node.GetChild("child"), &child);
}

TEST(DumpNodeTest, AddEntry) {
  auto global = std::make_unique<GlobalDump>();
  DumpNode node(global->global_container());

  node.AddEntry("test", DumpNode::Entry::Units::kBytes, 100ul);
  ASSERT_EQ(node.entries().size(), 1ul);

  auto entry = node.entries().find("test");
  ASSERT_EQ(entry->first, "test");
  ASSERT_EQ(entry->second.units, DumpNode::Entry::Units::kBytes);
  ASSERT_EQ(entry->second.value_uint64, 100ul);
}

}  // namespace memory_instrumentation
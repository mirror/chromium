// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/memory_instrumentation/graph.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace memory_instrumentation {

TEST(GlobalDumpTest, CreateContainerForProcess) {
  GlobalDump global_dump;

  ContainerDump* dump = global_dump.CreateContainerForProcess(10);
  ASSERT_TRUE(dump != nullptr);

  ASSERT_EQ(dump, global_dump.container_dumps().find(10)->second.get());
}

TEST(GlobalDumpTest, InsertNodeToGuidMap) {
  GlobalDump global_dump;
  DumpNode node(global_dump.global_container());

  MemoryAllocatorDumpGuid guid(10);
  global_dump.InsertNodeToGuidMap(guid, &node);
  ASSERT_EQ(&node, global_dump.nodes_by_guid().find(guid)->second);
}

TEST(ContainerDumpTest, CreateAndFindNodeInGraph) {
  GlobalDump global_dump;
  ContainerDump container(&global_dump);

  DumpNode* first = container.CreateNodeInGraph("complex/test/path/1");
  DumpNode* second = container.CreateNodeInGraph("complex/test/path/2");
  DumpNode* third = container.CreateNodeInGraph("complex/other/path/1");
  DumpNode* fourth = container.CreateNodeInGraph("simple/path");
  DumpNode* fifth = container.CreateNodeInGraph("simple/path/child/1");

  ASSERT_EQ(container.FindNodeInGraph("complex/test/path/1"), first);
  ASSERT_EQ(container.FindNodeInGraph("complex/test/path/2"), second);
  ASSERT_EQ(container.FindNodeInGraph("complex/other/path/1"), third);
  ASSERT_EQ(container.FindNodeInGraph("simple/path"), fourth);
  ASSERT_EQ(container.FindNodeInGraph("simple/path/child/1"), fifth);
}

TEST(DumpNodeTest, GetChild) {
  GlobalDump global_dump;
  DumpNode node(global_dump.global_container());

  ASSERT_EQ(node.GetChild("test"), nullptr);

  DumpNode child(global_dump.global_container());
  node.InsertChild("child", &child);
  ASSERT_EQ(node.GetChild("child"), &child);
}

TEST(DumpNodeTest, InsertChild) {
  GlobalDump global_dump;
  DumpNode node(global_dump.global_container());

  ASSERT_EQ(node.GetChild("test"), nullptr);

  DumpNode child(global_dump.global_container());
  node.InsertChild("child", &child);
  ASSERT_EQ(node.GetChild("child"), &child);
}

TEST(DumpNodeTest, AddEntry) {
  GlobalDump global_dump;
  DumpNode node(global_dump.global_container());

  node.AddEntry("scalar", DumpNode::Entry::Units::kBytes, 100ul);
  ASSERT_EQ(node.entries().size(), 1ul);

  node.AddEntry("string", DumpNode::Entry::Units::kObjects, "data");
  ASSERT_EQ(node.entries().size(), 2ul);

  auto scalar = node.entries().find("scalar");
  ASSERT_EQ(scalar->first, "scalar");
  ASSERT_EQ(scalar->second.units, DumpNode::Entry::Units::kBytes);
  ASSERT_EQ(scalar->second.value_uint64, 100ul);

  auto string = node.entries().find("string");
  ASSERT_EQ(string->first, "string");
  ASSERT_EQ(string->second.units, DumpNode::Entry::Units::kObjects);
  ASSERT_EQ(string->second.value_string, "data");
}

}  // namespace memory_instrumentation
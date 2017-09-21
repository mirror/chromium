// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/memory_instrumentation/graph.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace memory_instrumentation {

using base::trace_event::MemoryAllocatorDumpGuid;
using DumpGraph = memory_instrumentation::GlobalDumpGraph::DumpGraph;

TEST(GlobalDumpGraphTest, CreateContainerForProcess) {
  GlobalDumpGraph global_dump_graph;

  DumpGraph* dump = global_dump_graph.CreateGraphForProcess(10);
  ASSERT_TRUE(dump != nullptr);

  auto* map = global_dump_graph.process_dump_graphs().find(10)->second.get();
  ASSERT_EQ(dump, map);
}

TEST(DumpGraphTest, CreateAndFindNodeInGraph) {
  GlobalDumpGraph global_dump_graph;
  DumpGraph graph(&global_dump_graph);

  DumpNode* first =
      graph.CreateNodeInGraph(MemoryAllocatorDumpGuid(1), "simple/test/1");
  DumpNode* second =
      graph.CreateNodeInGraph(MemoryAllocatorDumpGuid(2), "simple/test/2");
  DumpNode* third =
      graph.CreateNodeInGraph(MemoryAllocatorDumpGuid(3), "simple/other/1");
  DumpNode* fourth =
      graph.CreateNodeInGraph(MemoryAllocatorDumpGuid(4), "complex/path");
  DumpNode* fifth = graph.CreateNodeInGraph(MemoryAllocatorDumpGuid(5),
                                            "complex/path/child/1");

  ASSERT_EQ(graph.FindNodeInGraph("simple/test/1"), first);
  ASSERT_EQ(graph.FindNodeInGraph("simple/test/2"), second);
  ASSERT_EQ(graph.FindNodeInGraph("simple/other/1"), third);
  ASSERT_EQ(graph.FindNodeInGraph("complex/path"), fourth);
  ASSERT_EQ(graph.FindNodeInGraph("complex/path/child/1"), fifth);

  auto& nodes_by_guid = global_dump_graph.nodes_by_guid();
  ASSERT_EQ(nodes_by_guid.find(MemoryAllocatorDumpGuid(1))->second, first);
  ASSERT_EQ(nodes_by_guid.find(MemoryAllocatorDumpGuid(2))->second, second);
  ASSERT_EQ(nodes_by_guid.find(MemoryAllocatorDumpGuid(3))->second, third);
  ASSERT_EQ(nodes_by_guid.find(MemoryAllocatorDumpGuid(4))->second, fourth);
  ASSERT_EQ(nodes_by_guid.find(MemoryAllocatorDumpGuid(5))->second, fifth);
}

TEST(DumpNodeTest, GetChild) {
  GlobalDumpGraph global_dump_graph;
  DumpNode node(global_dump_graph.shared_graph());

  ASSERT_EQ(node.GetChild("test"), nullptr);

  DumpNode child(global_dump_graph.shared_graph());
  node.InsertChild("child", &child);
  ASSERT_EQ(node.GetChild("child"), &child);
}

TEST(DumpNodeTest, InsertChild) {
  GlobalDumpGraph global_dump_graph;
  DumpNode node(global_dump_graph.shared_graph());

  ASSERT_EQ(node.GetChild("test"), nullptr);

  DumpNode child(global_dump_graph.shared_graph());
  node.InsertChild("child", &child);
  ASSERT_EQ(node.GetChild("child"), &child);
}

TEST(DumpNodeTest, AddEntry) {
  GlobalDumpGraph global_dump_graph;
  DumpNode node(global_dump_graph.shared_graph());

  node.AddEntry("scalar", DumpNode::Entry::ScalarUnits::kBytes, 100ul);
  ASSERT_EQ(node.entries().size(), 1ul);

  node.AddEntry("string", DumpNode::Entry::ScalarUnits::kObjects, "data");
  ASSERT_EQ(node.entries().size(), 2ul);

  auto scalar = node.entries().find("scalar");
  ASSERT_EQ(scalar->first, "scalar");
  ASSERT_EQ(scalar->second.units, DumpNode::Entry::ScalarUnits::kBytes);
  ASSERT_EQ(scalar->second.value_uint64, 100ul);

  auto string = node.entries().find("string");
  ASSERT_EQ(string->first, "string");
  ASSERT_EQ(string->second.units, DumpNode::Entry::ScalarUnits::kObjects);
  ASSERT_EQ(string->second.value_string, "data");
}

}  // namespace memory_instrumentation
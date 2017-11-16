// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/memory_instrumentation/graph.h"

#include "base/callback.h"
#include "base/strings/string_tokenizer.h"

namespace memory_instrumentation {

namespace {

using base::trace_event::MemoryAllocatorDumpGuid;
using Edge = GlobalDumpGraph::Edge;
using Process = GlobalDumpGraph::Process;
using Node = GlobalDumpGraph::Node;

}  // namespace

GlobalDumpGraph::GlobalDumpGraph()
    : shared_memory_graph_(
          std::make_unique<Process>(base::kNullProcessId, this)) {}
GlobalDumpGraph::~GlobalDumpGraph() {}

Process* GlobalDumpGraph::CreateGraphForProcess(base::ProcessId process_id) {
  auto id_to_dump_iterator = process_dump_graphs_.emplace(
      process_id, std::make_unique<Process>(process_id, this));
  DCHECK(id_to_dump_iterator.second);  // Check for duplicate pids
  return id_to_dump_iterator.first->second.get();
}

void GlobalDumpGraph::AddNodeOwnershipEdge(Node* owner,
                                           Node* owned,
                                           int importance) {
  // The owner node should not already be associated with an edge.
  DCHECK(!owner->owns_edge());

  all_edges_.emplace_front(owner, owned, importance);
  Edge* edge = &*all_edges_.begin();
  owner->SetOwnsEdge(edge);
  owned->AddOwnedByEdge(edge);
}

Node* GlobalDumpGraph::CreateNode(Process* process_graph, Node* parent) {
  all_nodes_.emplace_front(process_graph, parent);
  return &*all_nodes_.begin();
}

void GlobalDumpGraph::VisitInDepthFirstPostOrder(
    const base::RepeatingCallback<void(Node*)>& callback) {
  std::set<const Node*> visited;
  std::set<const Node*> path;
  shared_memory_graph()->root()->VisitInDepthFirstPostOrderRecursively(
      &visited, &path, callback);
  for (auto& pid_to_process : process_dump_graphs()) {
    pid_to_process.second->root()->VisitInDepthFirstPostOrderRecursively(
        &visited, &path, callback);
  }
}

Process::Process(base::ProcessId pid, GlobalDumpGraph* global_graph)
    : pid_(pid),
      global_graph_(global_graph),
      root_(global_graph->CreateNode(this, nullptr)) {}
Process::~Process() {}

Node* Process::CreateNode(MemoryAllocatorDumpGuid guid,
                          base::StringPiece path,
                          bool weak) {
  DCHECK(!path.empty());

  std::string path_string = path.as_string();
  base::StringTokenizer tokenizer(path_string, "/");

  // Perform a tree traversal, creating the nodes if they do not
  // already exist on the path to the child.
  Node* current = root_;
  while (tokenizer.GetNext()) {
    base::StringPiece key = tokenizer.token_piece();
    Node* parent = current;
    current = current->GetChild(key);
    if (!current) {
      current = global_graph_->CreateNode(this, parent);
      parent->InsertChild(key, current);
    }
  }

  // The final node should have the weakness specified by the
  // argument and also be considered explicit.
  current->set_weak(weak);
  current->set_explicit(true);

  // Add to the global guid map as well if it exists.
  if (!guid.empty())
    global_graph_->nodes_by_guid_.emplace(guid, current);

  return current;
}

Node* Process::FindNode(base::StringPiece path) {
  DCHECK(!path.empty());

  std::string path_string = path.as_string();
  base::StringTokenizer tokenizer(path_string, "/");
  Node* current = root_;
  while (tokenizer.GetNext()) {
    base::StringPiece key = tokenizer.token_piece();
    current = current->GetChild(key);
    if (!current)
      return nullptr;
  }
  return current;
}

Node::Node(Process* dump_graph, Node* parent)
    : dump_graph_(dump_graph), parent_(parent), owns_edge_(nullptr) {}
Node::~Node() {}

Node* Node::GetChild(base::StringPiece name) {
  DCHECK(!name.empty());
  DCHECK_EQ(std::string::npos, name.find('/'));

  auto child = children_.find(name.as_string());
  return child == children_.end() ? nullptr : child->second;
}

void Node::InsertChild(base::StringPiece name, Node* node) {
  DCHECK(!name.empty());
  DCHECK_EQ(std::string::npos, name.find('/'));

  children_.emplace(name.as_string(), node);
}

Node* Node::CreateChild(base::StringPiece name) {
  Node* new_child = dump_graph_->global_graph()->CreateNode(dump_graph_, this);
  InsertChild(name, new_child);
  return new_child;
}

bool Node::IsDescendentOf(const Node& possible_parent) const {
  const Node* current = this;
  while (current != nullptr) {
    if (current == &possible_parent)
      return true;
    current = current->parent();
  }
  return false;
}

void Node::AddOwnedByEdge(Edge* edge) {
  owned_by_edges_.push_back(edge);
}

void Node::SetOwnsEdge(Edge* owns_edge) {
  owns_edge_ = owns_edge;
}

void Node::AddEntry(std::string name,
                    Node::Entry::ScalarUnits units,
                    uint64_t value) {
  entries_.emplace(name, Node::Entry(units, value));
}

void Node::AddEntry(std::string name, std::string value) {
  entries_.emplace(name, Node::Entry(value));
}

void Node::VisitInDepthFirstPostOrderRecursively(
    std::set<const Node*>* visited,
    std::set<const Node*>* path,
    const base::RepeatingCallback<void(Node*)>& callback) {
  // If the node has already been visited, don't visit it again.
  if (visited->find(this) != visited->end())
    return;

  // Check that the node has not already been encountered on the current path.
  DCHECK(path->find(this) == path->end());
  path->insert(this);

  // Visit all owners of this node.
  for (auto* edge : *owned_by_edges()) {
    edge->source()->VisitInDepthFirstPostOrderRecursively(visited, path,
                                                          callback);
  }

  // Visit all children of this node.
  for (auto name_to_child : *children()) {
    name_to_child.second->VisitInDepthFirstPostOrderRecursively(visited, path,
                                                                callback);
  }

  // Visit the current node itself.
  callback.Run(this);
  visited->insert(this);

  // The current node is no longer on the path.
  path->erase(this);
}

Node::Entry::Entry(Entry::ScalarUnits units, uint64_t value)
    : type(Node::Entry::Type::kUInt64), units(units), value_uint64(value) {}

Node::Entry::Entry(std::string value)
    : type(Node::Entry::Type::kString),
      units(Node::Entry::ScalarUnits::kObjects),
      value_string(value),
      value_uint64(0) {}

Edge::Edge(Node* source, Node* target, int priority)
    : source_(source), target_(target), priority_(priority) {}

}  // namespace memory_instrumentation
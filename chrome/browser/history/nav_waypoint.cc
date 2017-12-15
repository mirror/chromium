// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_waypoint.h"

#include <ostream>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/history/nav_tree_node.h"
#include "chrome/browser/history/nav_tree_page.h"

NavWaypoint::NavWaypoint(Type t, NavTreeNode* node) : type_(t), node_(node) {}
NavWaypoint::~NavWaypoint() {}

const base::string16& NavWaypoint::GetTitle() const {
  // Use our own title for search nodes, otherwise just return the page one.
  if (!node_ || type_ == Type::kSearch)
    return TreeNode::GetTitle();
  return node_->page()->title();
}

void NavWaypoint::SetTitle(const base::string16& title) {
  // Only searches should have custom titles.
  DCHECK(type_ == Type::kSearch);
  TreeNode::SetTitle(title);
}

void NavWaypoint::DebugPrint(std::ostream& out, int indent_spaces) {
  constexpr size_t kMaxTitle = 32;

  out << std::string(indent_spaces, ' ') << "[" << TypeToString(type_) << "]";
  if (node_) {
    std::string title = base::UTF16ToUTF8(GetTitle());
    if (title.size() > kMaxTitle) {
      title.resize(kMaxTitle - 3);
      title.append("...");
    }
    out << " \"" << title << "\" " << node_->page()->url().spec();
  }
  out << std::endl;

  for (int i = 0; i < child_count(); i++)
    GetChild(i)->DebugPrint(out, indent_spaces + 2);
}

// static
const char* NavWaypoint::TypeToString(Type t) {
  switch (t) {
    case Type::kRoot:
      return "Root";
    case Type::kBack:
      return "Back";
    case Type::kSearch:
      return "Search";
    case Type::kSearchResult:
      return "Result";
    case Type::kHub:
      return "Hub";
    default:
      return nullptr;
  }
}

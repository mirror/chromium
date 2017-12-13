// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_jaunt.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/history/nav_tree_node.h"
#include "chrome/browser/history/nav_tree_page.h"
#include "net/base/url_util.h"

namespace {

// If the given URL is for a search results page, returns the search terms.
// Otherwise returns empty string. Should possibly integrate with
// //components/search_engines.
std::string ExtractSearchTerms(const GURL& url) {
  // Hardcoded detection for search results pages.
  // TODO(brettw) generalize this.
  if (url.host_piece() != "www.google.com" || url.path_piece() != "/search")
    return std::string();

  constexpr char kQueryKey[] = "q";
  net::QueryIterator query_iter(url);
  while (!query_iter.IsAtEnd()) {
    if (query_iter.GetKey() == kQueryKey)
      return query_iter.GetUnescapedValue();
  }
  return std::string();
}

}  // namespace

NavJaunt::NavJaunt()
    : TreeNodeModel(
          std::make_unique<NavWaypoint>(NavWaypoint::Type::kRoot, nullptr)) {}

NavJaunt::~NavJaunt() {}

void NavJaunt::SetCurrent(NavTreeNode* current, ChangeType change) {
  NavWaypoint* root = GetRoot();
  root->set_node(current);
  UpdateBackNode();
  if (change == ChangeType::kNew) {
    if (!AddSearchIfNecessary())
      AddSearchResultIfNecessary();
  }
}

void NavJaunt::UpdateBackNode() {
  // Update the persistent back node, which is the first child if present.
  NavWaypoint* root = GetRoot();
  NavTreeNode* new_back_node = root->node()->from();  // May be null.
  if (has_back_node_ && new_back_node) {
    // Update existing back node.
    NavWaypoint* back_waypoint = root->GetChild(0);
    DCHECK(back_waypoint->type() == NavWaypoint::Type::kBack);
    back_waypoint->set_node(new_back_node);
    NotifyObserverTreeNodeChanged(back_waypoint);
  } else if (has_back_node_) {
    // Back node exists but it's not needed any more.
    Remove(root, 0);
    has_back_node_ = false;
  } else if (new_back_node) {
    // No back node exists but it's now needed.
    Add(root,
        std::make_unique<NavWaypoint>(NavWaypoint::Type::kBack, new_back_node),
        0);
    has_back_node_ = true;
  }
}

bool NavJaunt::AddSearchIfNecessary() {
  NavWaypoint* root = GetRoot();
  NavTreeNode* node = root->node();
  const GURL& url = node->page()->url();

  std::string terms = ExtractSearchTerms(url);
  if (terms.empty())
    return false;

  // Index where the first search result should go.
  int search_node_index = has_back_node_ ? 1 : 0;

  auto found_search = searches_.find(terms);
  if (found_search == searches_.end()) {
    // Make a new search node.
    auto search_waypoint =
        std::make_unique<NavWaypoint>(NavWaypoint::Type::kSearch, node);
    search_waypoint->SetTitle(base::UTF8ToUTF16(terms));

    searches_.emplace(terms, search_waypoint.get());
    Add(root, std::move(search_waypoint), search_node_index);
  } else {
    // Have an existing search node for these terms. It's possible to do more
    // that one visit to the same search, and the URLs could be slightly
    // different (This is easy to do on Google: do a search you've never done
    // in the omnibox, then do that search again. The URL will be different
    // because it's actually a search history suggestion.)
    NavWaypoint* search_waypoint = found_search->second;
    if (search_waypoint->node() != node) {
      // Update the search node to point to the latest version of the URL.
      search_waypoint->set_node(node);
      NotifyObserverTreeNodeChanged(search_waypoint);
    }
  }
  return true;
}

bool NavJaunt::AddSearchResultIfNecessary() {
  // This is a search result if the referrer is a search page.
  NavWaypoint* root = GetRoot();
  NavTreeNode* node = root->node();
  if (!node->from())
    return false;

  const GURL& cur_url = node->page()->url();
  const GURL& from_url = node->from()->page()->url();

  // It would be nice if we could do this more efficiently than re-computing
  // the referring page from first principles. It would be nice to check the
  // top level of nodes in this object for node->from(), but it's possible to
  // get more than one node for the same search, and the test can fail.
  //
  // It's probably best if the NavTreeNodes maintain inside of them whether
  // each one was a search page so this can be done only once globally.
  std::string terms = ExtractSearchTerms(from_url);
  if (terms.empty())
    return false;

  auto found_search = searches_.find(terms);
  if (found_search == searches_.end()) {
    // Search should have been known in advance. Maybe this can happen in
    // cross-tab navigations?
    NOTREACHED();
    return false;
  }

  // See if the search node already has an entry for this URL. This will happen
  // if the user visits the same URL from the same search via clicking multiple
  // times.
  NavWaypoint* search_waypoint = found_search->second;
  for (int i = 0; i < search_waypoint->child_count(); i++) {
    NavWaypoint* cur_child = search_waypoint->GetChild(i);
    if (cur_child->node()->page()->url() == cur_url)
      return true;  // Found already present.
  }

  // Add a new search result node.
  search_waypoint->Add(
      std::make_unique<NavWaypoint>(NavWaypoint::Type::kSearchResult, node),
      search_waypoint->child_count());
  return true;
}

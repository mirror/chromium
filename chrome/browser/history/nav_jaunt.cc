// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_jaunt.h"

#include <algorithm>
#include <map>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/history/nav_tree.h"
#include "chrome/browser/history/nav_tree_factory.h"
#include "chrome/browser/history/nav_tree_node.h"
#include "chrome/browser/history/nav_tree_page.h"
#include "chrome/browser/history/nav_tree_traverse.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
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

struct WaypointNodeMaxTimeGreater {
  bool operator()(const std::unique_ptr<NavWaypoint>& a,
                  const std::unique_ptr<NavWaypoint>& b) const {
    return a->max_time() > b->max_time();
  }
};

// Creates a set of search waypoints for all searches in the given set. The
// results of each search will be results in the tree. It will be sorted
// most-recent-first.
std::vector<std::unique_ptr<NavWaypoint>> CreateSearchWaypoints(
    const base::flat_set<NavTreeNode*>& nodes) {
  // Find search nodes. The same search terms could have multiple NavTreeNodes.
  base::flat_map<std::string, std::vector<NavTreeNode*>> search_nodes;
  for (NavTreeNode* node : nodes) {
    std::string terms = ExtractSearchTerms(node->page()->url());
    if (!terms.empty())
      search_nodes[terms].push_back(node);
  }

  // Construct search nodes for each search.
  std::vector<std::unique_ptr<NavWaypoint>> results;
  for (auto& pair : search_nodes) {
    // Choose the most recent seach node as the root.
    // TODO(brettw) do something if the searches are different types, e.g. web,
    // images, shopping site.
    auto last_search = std::max_element(pair.second.begin(), pair.second.end(),
                                        NavTreeNodeTimeLess());
    auto search_waypoint =
        std::make_unique<NavWaypoint>(NavWaypoint::Type::kSearch, *last_search);
    search_waypoint->SetTitle(base::UTF8ToUTF16(pair.first));

    // Collect all children of the searches.
    std::vector<NavTreeNode*> children;
    for (NavTreeNode* search : pair.second) {
      for (auto& result : search->to())
        children.push_back(result.get());
    }

    // Add the results so the most recent one is first.
    std::sort(children.begin(), children.end(), NavTreeNodeTimeGreater());
    for (NavTreeNode* child : children) {
      search_waypoint->Add(std::make_unique<NavWaypoint>(
                               NavWaypoint::Type::kSearchResult, child),
                           search_waypoint->child_count());
    }

    search_waypoint->ComputeMinMaxTime();
    results.push_back(std::move(search_waypoint));
  }

  // Reverse-sort the search waypoints so the most recent search is first.
  std::sort(results.begin(), results.end(), WaypointNodeMaxTimeGreater());
  return results;
}

std::vector<std::unique_ptr<NavWaypoint>> ClassifyByDomain(
    const base::flat_set<NavTreeNode*>& nodes) {
  std::map<std::string, std::vector<NavTreeNode*>> domains;
  for (NavTreeNode* node : nodes) {
    std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
        node->page()->url(),
        net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
    domains[domain].push_back(node);
  }

  NavTreeNodeTimeLess less_cmp;
  std::vector<std::unique_ptr<NavWaypoint>> results;

  for (auto& domain_pair : domains) {
    // Sort with the most recent first.
    std::vector<NavTreeNode*>& pages_in_domain = domain_pair.second;
    std::sort(pages_in_domain.begin(), pages_in_domain.end(),
              NavTreeNodeTimeGreater());

    // Find the representative node for pages in this domain. This is either
    // the first one with no path or the first one otherwise.
    NavTreeNode* no_path_node = nullptr;
    for (auto* node : domain_pair.second) {
      const GURL& url = node->page()->url();
      if (url.path_piece().size() <= 1) {  // "/" is a normal "empty path".
        // Take the new one if an old one doesn't exist or the new time is
        // smaller (older).
        if (!no_path_node || less_cmp(node, no_path_node))
          no_path_node = node;
      }
    }
    NavTreeNode* selected_root =
        no_path_node ? no_path_node : pages_in_domain.back();

    auto domain_waypoint = std::make_unique<NavWaypoint>(
        NavWaypoint::Type::kDomainGroup, selected_root);

    // Add all other nodes as children. They are already sorted.
    for (auto* node : domain_pair.second) {
      if (node != selected_root) {
        domain_waypoint->Add(
            std::make_unique<NavWaypoint>(NavWaypoint::Type::kPage, node),
            domain_waypoint->child_count());
      }
    }

    domain_waypoint->ComputeMinMaxTime();
    results.push_back(std::move(domain_waypoint));
  }

  // Reverse-sort the search waypoints so the most recent search is first.
  std::sort(results.begin(), results.end(), WaypointNodeMaxTimeGreater());
  return results;
}

// Returns wiether the items in the given waypoint all belong to the given set.
// Note that the waypoint itself counts as an item, as well as its children.
bool AllItemsInSet(const NavWaypoint* waypoint,
                   const base::flat_set<const NavTreeNode*>& set) {
  if (set.find(waypoint->node()) == set.end())
    return false;
  for (int i = 0; i < waypoint->child_count(); i++) {
    if (set.find(waypoint->GetChild(i)->node()) == set.end())
      return false;
  }
  return true;
}

struct BuildState {
  std::map<GURL, NavTreeNode> urls_;
  std::set<NavTreeNode*> seen_nodes_;
};

}  // namespace

NavJaunt::NavJaunt()
    : TreeNodeModel(
          std::make_unique<NavWaypoint>(NavWaypoint::Type::kRoot, nullptr)) {}

NavJaunt::~NavJaunt() {}

void NavJaunt::InitFromCurrentState(NavTree* tree,
                                    const content::WebContents* contents) {
  // Allow multiple-init.
  NavWaypoint* root_waypoint = GetRoot();
  root_waypoint->DeleteAll();

  scoped_refptr<NavTreeNode> nav_root;
  scoped_refptr<NavTreeNode> active;
  if (!tree->NodesForTab(contents, &nav_root, &active))
    return;

  auto all_nodes = NavTreeGetConnectedNodes(nav_root.get());
  std::vector<std::unique_ptr<NavWaypoint>> search_waypoints =
      CreateSearchWaypoints(all_nodes);
  std::vector<std::unique_ptr<NavWaypoint>> domain_classification =
      ClassifyByDomain(all_nodes);

  // Collect sets of the classified nodes.
  base::flat_set<const NavTreeNode*> search_nodes;
  base::flat_set<const NavTreeNode*> search_result_nodes;
  for (const auto& search : search_waypoints) {
    search_nodes.insert(search->node());
    for (int i = 0; i < search->child_count(); i++)
      search_result_nodes.insert(search->GetChild(i)->node());
  }

  // Remove search pages (for example, google.com/search...) from the domain
  // classifications when they don't have any children. These will look weirdly
  // duplicated and we don't want a google.com domain grouping unless the user
  // browsed google.com other than searching.
  //
  // TODO(brettw) this doesn't handle the case where there are more than one
  // search node for the same search. If this happens, the other one may look
  // duplicated.
  for (auto& domain : domain_classification) {
    for (int i = 0; i < domain->child_count(); i++) {
      NavWaypoint* waypoint = domain->GetChild(i);
      if (search_nodes.find(waypoint->node()) != search_nodes.end()) {
        domain->Remove(i);
        i--;
      }
    }
  }

  // Remove some domain classifications that are duplicates.
  for (int i = 0; i < static_cast<int>(domain_classification.size()); i++) {
    const NavWaypoint* domain_waypoint = domain_classification[i].get();
    if ((search_nodes.find(domain_waypoint->node()) != search_nodes.end() &&
         domain_waypoint->child_count() == 0) ||
        AllItemsInSet(domain_waypoint, search_result_nodes)) {
      domain_classification.erase(domain_classification.begin() + i);
      i--;
    }
  }

  // Merge the two lists.
  std::vector<std::unique_ptr<NavWaypoint>> merged =
      std::move(search_waypoints);
  merged.insert(merged.end(),
                std::make_move_iterator(domain_classification.begin()),
                std::make_move_iterator(domain_classification.end()));
  std::sort(merged.begin(), merged.end(), WaypointNodeMaxTimeGreater());

  // Add searches to the root.
  for (auto& item : merged)
    root_waypoint->Add(std::move(item), root_waypoint->child_count());
}

void NavJaunt::SetCurrent(NavTreeNode* current, ChangeType change) {
  NavWaypoint* root = GetRoot();
  root->set_node(current);
  UpdateBackNode();
  if (change == ChangeType::kNew) {
    if (!AddSearchIfNecessary())
      AddSearchResultIfNecessary();
  }
}

void NavJaunt::OnNodeAdded(NavTreeNode* node, bool is_active) {}

void NavJaunt::OnActiveNodeChanged(NavTreeNode* new_active) {
  active_node_ = new_active;
}

void NavJaunt::OnNodeDataChanged(NavTreeNode* node) {}

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

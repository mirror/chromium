// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_tree.h"

#include <algorithm>
#include <fstream>

#include "base/containers/circular_deque.h"
#include "base/containers/flat_set.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/history/nav_tree_factory.h"
#include "chrome/browser/history/nav_tree_node.h"
#include "chrome/browser/history/nav_tree_page.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/content/browser/history_context_helper.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace {

bool IsSearchResultsPage(const NavTreeNode* node) {
  // Hardcoded detection for search results pages.
  // TODO(brettw) generalize this.
  const GURL& node_url = node->oage().url();
  return node_url.host_piece() == "www.google.com" &&
      node_url.path_piece() == "/search";
}

NavTree::WatpointType ClassifyWaypoint(const NavTreeNode* node) {
  if (IsSearchResultsPage(node))
    return WaypointType::kSearch;

  // Anything coming from a search results page is a result.
  if (node->from() && IsSearchResultsPage(node->from()))
    return WaypointType::kSearchResult;

  // Any node with more than one link click out of it.
  if (node->to().size() > 1)
    return WaypointType::kHub;

  return WaypointType::kNone;
}

void RecursivePrintTree(std::ostream& out,
                        int indent_spaces,
                        NavTreeNode* node,
                        NavTreeNode* mark_node) {
  out << std::string(indent_spaces, ' ');
  if (node == mark_node)
    out << "#### ";
  out << node->page()->url().spec() << " 0x" << std::hex
      << node->transition_in() << " \""
      << base::UTF16ToUTF8(node->page()->title()) << "\"\n";
  for (NavTreeNode* to : node->to())
    RecursivePrintTree(out, indent_spaces + 2, to, mark_node);
}

// Returns all NavTreeNodes reachable from the
base::flat_set<NavTreeNode*> GetConnectedNodes(NavTreeNode* start) {
  base::flat_set<NavTreeNode*> result;

  base::circular_deque<NavTreeNode*> queue;
  // Avoid many small allocations, this memory is thrown away immediately so
  // wasted space doesn't matter much.
  queue.reserve(32);
  queue.push_back(start);

  while (!queue.empty()) {
    // Everything in the queue has already been added to the result.
    NavTreeNode* cur = queue.front();
    queue.pop_front();

    if (cur->from() && result.count(cur->from()) == 0) {
      result.insert(cur->from());
      queue.push_back(cur->from());
    }

    for (NavTreeNode* to : cur->to()) {
      if (result.count(to) == 0) {
        result.insert(to);
        queue.push_back(to);
      }
    }
  }

  return result;
}

}  // namespace

NavTree::NavTree(Profile* profile)
    : profile_(profile), tab_strip_model_observer_(this) {}

NavTree::~NavTree() {
  log_.clear();

  // NavTreePage leaking. This also indicates a dangling back pointer.
  DCHECK(live_pages_.empty());
}

// static
const base::Feature NavTree::kFeature{
    "NavTree", base::FEATURE_DISABLED_BY_DEFAULT,
};

// static
bool NavTree::IsEnabled() {
  return base::FeatureList::IsEnabled(kFeature);
}

std::vector<GURL> NavTree::GetWaypoints(
    content::WebContents* web_contents) const {
  std::vector<GURL> result;

  auto found_head = heads_.find(web_contents);
  if (found_head == heads_.end())
    return result;

  NavTreeNode* current = found_head->second;

  // Tracks all nodes visited as we traverse the graph.
  // std::set<NavTreeNode*> visited;
  // visited.push_back(current);

  // Always include the previous page.
  if (current->from())
    result.push_back(current->from()->page()->url());

  // TODO(brettw) walk up and down chain.

  return result;
}

NavTreeNode* NavTree::NodeForNavigationEntry(
    const content::WebContents* web_contents,
    const content::NavigationEntry* entry) const {
  // The const_cast required for comparison, but the WebContents remains
  // unchanged.
  auto found_web_contents =
      heads_.find(const_cast<content::WebContents*>(web_contents));
  if (found_web_contents == heads_.end())
    return nullptr;

  int entry_id = entry->GetUniqueID();
  for (NavTreeNode* cur : GetConnectedNodes(found_web_contents->second)) {
    if (cur->nav_entry_id() == entry_id)
      return cur;
  }
  return nullptr;
}

void NavTree::OnNavigate(content::NavigationHandle* nav_handle) {
  if (!nav_handle->IsInMainFrame())
    return;  // For now, ignore all non-main-frame navigations.

  const GURL& url = nav_handle->GetURL();
  content::WebContents* web_contents = nav_handle->GetWebContents();
  const content::NavigationEntry* nav_entry =
      web_contents->GetController().GetLastCommittedEntry();
  int nav_entry_id = nav_entry->GetUniqueID();

  scoped_refptr<NavTreePage> page = GetOrCreatePage(url);
  UpdatePageForNavigation(nav_entry, page.get());

  // Find referring node.
  NavTreeNode* referring_node = nullptr;
  auto found_web_contents = heads_.find(web_contents);
  if (found_web_contents != heads_.end()) {
    if (nav_entry_id > largest_nav_entry_id_) {
      // The nav entry IDs increase so seeing a bigger one than we've seen
      // before means it's a new entry.
      referring_node = found_web_contents->second;
    } else {
      // This is an old ID so it's a renavigation, try to find which one from
      // the set of navigations in this tab.
      NavTreeNode* node = NodeForNavigationEntry(web_contents, nav_entry);
      DCHECK(node);  // Should always find a renavigation.
      heads_[web_contents] = node;
    }
  }
  // We should always have found an entry for replaced ones.
  DCHECK(!nav_handle->DidReplaceEntry());

  std::unique_ptr<NavTreeNode> node = std::make_unique<NavTreeNode>(
      referring_node, page, nav_entry->GetUniqueID(),
      nav_handle->GetPageTransition());

  heads_[web_contents] = node.get();
  largest_nav_entry_id_ = std::max(largest_nav_entry_id_, nav_entry_id);

  if (referring_node)
    referring_node->AddTo(node.get());

  log_.push_back(std::move(node));
  DebugDumpNavTreeToFile();
}

void NavTree::UpdateTitleForPage(const content::WebContents* contents,
                                 const content::NavigationEntry* nav_entry) {
  NavTreeNode* node = NodeForNavigationEntry(contents, nav_entry);
  if (!node)
    return;
  const base::string16& title = nav_entry->GetTitle();
  if (!title.empty())
    node->page()->set_title(title);  // Don't clear an existing one.
}

void NavTree::WebContentsDestroyed(content::WebContents* web_contents) {
  auto found = heads_.find(web_contents);
  if (found == heads_.end())
    return;
  heads_.erase(found);
}

void NavTree::NavTreePageDestroyed(NavTreePage* page) {
  auto found = live_pages_.find(page->url());
  DCHECK(found != live_pages_.end());
  live_pages_.erase(found);
}

void NavTree::Shutdown() {}

scoped_refptr<NavTreePage> NavTree::GetOrCreatePage(const GURL& url) {
  NavTreePage*& page_ptr = live_pages_[url];
  if (page_ptr)
    return scoped_refptr<NavTreePage>(page_ptr);

  scoped_refptr<NavTreePage> new_page(new NavTreePage(this, url));
  page_ptr = new_page.get();
  return new_page;
}

void NavTree::UpdatePageForNavigation(const content::NavigationEntry* nav_entry,
                                      NavTreePage* page) {
  const base::string16& title = nav_entry->GetTitle();
  if (!title.empty())
    page->set_title(title);  // Don't overwrite an existing one with nothing.
}

void NavTree::DebugDumpNavTreeToFile() {
  std::ofstream out("P:\\nav_tree.txt", std::ios::out | std::ios::app);

  out << "\n==========================\nLOG\n";
  for (const auto& cur : log_)
    out << "  " << cur->page()->url().spec() << "\n";

  out << "TREE (indents go back in time)\n";
  for (const auto& head : heads_) {
    // Walk backwards to find the root of the tree.
    NavTreeNode* head_root = head.second;
    while (head_root->from())
      head_root = head_root->from();
    RecursivePrintTree(out, 2, head_root, head.second);
  }
}

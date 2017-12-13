// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_tree.h"

#include <algorithm>
#include <fstream>

#include "base/containers/circular_deque.h"
#include "base/containers/flat_set.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/history/nav_jaunt.h"
#include "chrome/browser/history/nav_tree_factory.h"
#include "chrome/browser/history/nav_tree_node.h"
#include "chrome/browser/history/nav_tree_page.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/content/browser/history_context_helper.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace {

void RecursivePrintTree(std::ostream& out,
                        int indent_spaces,
                        const NavTreeNode* node,
                        const NavTreeNode* mark_node) {
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
  result.insert(start);
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

NavTreeNode* NavTree::NodeForNavigationEntry(
    const content::WebContents* web_contents,
    const content::NavigationEntry* entry) {
  // The const_cast required for comparison, but the WebContents remains
  // unchanged.
  auto found_web_contents =
      heads_.find(const_cast<content::WebContents*>(web_contents));
  if (found_web_contents == heads_.end())
    return nullptr;

  int entry_id = entry->GetUniqueID();
  for (NavTreeNode* cur :
       GetConnectedNodes(found_web_contents->second.root_node())) {
    if (cur->nav_entry_id() == entry_id)
      return cur;
  }
  return nullptr;
}

NavJaunt* NavTree::JauntForWebContents(const content::WebContents* contents) {
  auto found_web_contents =
      heads_.find(const_cast<content::WebContents*>(contents));
  if (found_web_contents == heads_.end())
    return nullptr;
  return &found_web_contents->second;
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
      referring_node = found_web_contents->second.root_node();
    } else {
      // This is an old ID so it's a renavigation, try to find which one from
      // the set of navigations in this tab.
      NavTreeNode* node = NodeForNavigationEntry(web_contents, nav_entry);
      if (node) {
        found_web_contents->second.SetCurrent(node, NavJaunt::ChangeType::kOld);
        DebugDumpNavTreeToFile();
        return;
      }
      // Otherwise this is a renavigation to another tab.
      // TODO(brettw) figure out when this triggers.
    }
  }

  std::unique_ptr<NavTreeNode> node = std::make_unique<NavTreeNode>(
      referring_node, page, nav_entry->GetUniqueID(),
      nav_handle->GetPageTransition());

  if (found_web_contents == heads_.end()) {
    found_web_contents = heads_
                             .emplace(std::piecewise_construct,
                                      std::forward_as_tuple(web_contents),
                                      std::forward_as_tuple())
                             .first;
    found_web_contents->second.SetCurrent(node.get(),
                                          NavJaunt::ChangeType::kNew);
  } else {
    found_web_contents->second.SetCurrent(node.get(),
                                          NavJaunt::ChangeType::kNew);
  }
  largest_nav_entry_id_ = std::max(largest_nav_entry_id_, nav_entry_id);

  if (referring_node)
    referring_node->AddTo(node.get());

  log_.push_back(std::move(node));
  DebugDumpNavTreeToFile();
}

void NavTree::OnNavigationEntryUpdated(const content::WebContents* contents,
                                       const content::NavigationEntry* entry) {
  NavTreeNode* node = NodeForNavigationEntry(contents, entry);
  if (!node)
    return;

  // Update title.
  const base::string16& title = entry->GetTitle();
  if (!title.empty())
    node->page()->set_title(title);  // Don't clear an existing one.

  // Update favicon.
  node->page()->set_favicon(entry->GetFavicon());
}

void NavTree::UpdateTitleForPage(const content::WebContents* contents,
                                 const content::NavigationEntry* nav_entry) {
  /*
NavTreeNode* node = NodeForNavigationEntry(contents, nav_entry);
if (!node)
return;
const base::string16& title = nav_entry->GetTitle();
if (!title.empty())
node->page()->set_title(title);  // Don't clear an existing one.
*/
}

void NavTree::WebContentsDestroyed(content::WebContents* web_contents) {
  // TODO(brettw) if a model is attached to the Jaunt associated with the
  // WebContents, I'm not sure about the destruction order and whether this
  // is safe or could cause a use-after-free.
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

  page->set_favicon(nav_entry->GetFavicon());
}

void NavTree::DebugDumpNavTreeToFile() {
  // std::ofstream out("P:\\nav_tree.txt", std::ios::out | std::ios::app);
  std::ofstream out("//usr/local/google/home/brettw/nav_tree.txt",
                    std::ios::out | std::ios::app);

  out << "\n==========================\nLOG\n";
  for (const auto& cur : log_)
    out << "  " << cur->page()->url().spec() << "\n";

  for (auto& head : heads_) {
    // Walk backwards to find the root of the tree.
    NavTreeNode* head_root = head.second.root_node();
    while (head_root->from())
      head_root = head_root->from();

    out << "TAB\n  TREE\n";
    RecursivePrintTree(out, 4, head_root, head.second.root_node());
    out << "  JAUNT\n";
    head.second.GetRoot()->DebugPrint(out, 4);
  }
}

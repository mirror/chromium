// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_tree.h"

#include <algorithm>
#include <fstream>

#include "base/containers/circular_deque.h"
#include "base/containers/flat_set.h"
#include "base/observer_list.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/history/nav_jaunt.h"
#include "chrome/browser/history/nav_tree_factory.h"
#include "chrome/browser/history/nav_tree_node.h"
#include "chrome/browser/history/nav_tree_observer.h"
#include "chrome/browser/history/nav_tree_page.h"
#include "chrome/browser/history/nav_tree_traverse.h"
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
  for (const auto& to : node->to())
    RecursivePrintTree(out, indent_spaces + 2, to.get(), mark_node);
}

}  // namespace

struct NavTree::TabData {
  // The first navigation from this tab. Nodes are owned by the referring node
  // so this reference keeps the full tree tof this tab in memory.
  scoped_refptr<NavTreeNode> root;

  // The current page in the tab.
  scoped_refptr<NavTreeNode> active;

  // Observers for changes related to this tab.
  base::ObserverList<NavTreeObserver> observers;
};

NavTree::NavTree(Profile* profile)
    : profile_(profile), tab_strip_model_observer_(this) {}

NavTree::~NavTree() {}

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
      tabs_.find(const_cast<content::WebContents*>(web_contents));
  if (found_web_contents == tabs_.end())
    return nullptr;

  int entry_id = entry->GetUniqueID();
  for (NavTreeNode* cur :
       NavTreeGetConnectedNodes(found_web_contents->second.root.get())) {
    if (cur->nav_entry_id() == entry_id)
      return cur;
  }
  return nullptr;
}

bool NavTree::NodesForTab(const content::WebContents* contents,
                          scoped_refptr<NavTreeNode>* root,
                          scoped_refptr<NavTreeNode>* active) {
  auto found_web_contents =
      tabs_.find(const_cast<content::WebContents*>(contents));
  if (found_web_contents == tabs_.end())
    return false;

  *root = found_web_contents->second.root;
  *active = found_web_contents->second.active;
  return true;
}

NavJaunt* NavTree::JauntForWebContents(const content::WebContents* contents) {
  /*
    auto found_web_contents =
        tabs_.find(const_cast<content::WebContents*>(contents));
    if (found_web_contents == tabs_.end())
      return nullptr;
    return &found_web_contents->second;
    */
  return nullptr;
}

void NavTree::AddTabObserver(const content::WebContents* current_contents,
                             NavTreeObserver* observer) {
  auto found_web_contents =
      tabs_.find(const_cast<content::WebContents*>(current_contents));
  if (found_web_contents == tabs_.end()) {
    NOTREACHED();
    return;
  }
  found_web_contents->second.observers.AddObserver(observer);
}

void NavTree::RemoveTabObserver(const content::WebContents* current_contents,
                                NavTreeObserver* observer) {
  auto found_web_contents =
      tabs_.find(const_cast<content::WebContents*>(current_contents));
  if (found_web_contents == tabs_.end()) {
    NOTREACHED();
    return;
  }
  found_web_contents->second.observers.RemoveObserver(observer);
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
  auto found_web_contents = tabs_.find(web_contents);
  if (found_web_contents != tabs_.end()) {
    TabData& tab_data = found_web_contents->second;

    if (nav_entry_id > largest_nav_entry_id_) {
      // The nav entry IDs increase so seeing a bigger one than we've seen
      // before means it's a new entry.
      referring_node = tab_data.active.get();
    } else {
      // This is an old ID so it's a renavigation, try to find which one from
      // the set of navigations in this tab.
      NavTreeNode* node = NodeForNavigationEntry(web_contents, nav_entry);
      if (node) {
        tab_data.active = node;
        for (auto& observer : tab_data.observers)
          observer.OnNodeAdded(node, true);

        DebugDumpNavTreeToFile();
        return;
      }
      // Otherwise this is a renavigation to another tab.
      // TODO(brettw) figure out when this triggers.
    }
  }

  scoped_refptr<NavTreeNode> node = base::MakeRefCounted<NavTreeNode>(
      referring_node, page, nav_entry->GetUniqueID(),
      nav_handle->GetPageTransition());

  if (found_web_contents == tabs_.end()) {
    found_web_contents = tabs_
                             .emplace(std::piecewise_construct,
                                      std::forward_as_tuple(web_contents),
                                      std::forward_as_tuple())
                             .first;
    found_web_contents->second.root = node.get();
  }
  found_web_contents->second.active = node.get();
  largest_nav_entry_id_ = std::max(largest_nav_entry_id_, nav_entry_id);

  if (referring_node)
    referring_node->AddTo(node.get());

  // Notify only after all bookkeeping has been updated.
  for (auto& observer : found_web_contents->second.observers)
    observer.OnNodeAdded(node.get(), true);

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

  // TODO(brettw) this page could be referenced by more than one node which
  // should now be counted as changed. We will need to notify for all of them
  // in which case the observer should probably take a span of nodes that
  // changed or something.
  auto found_web_contents =
      tabs_.find(const_cast<content::WebContents*>(contents));
  if (found_web_contents != tabs_.end()) {
    for (auto& observer : found_web_contents->second.observers)
      observer.OnNodeDataChanged(node);
  }
}

void NavTree::WebContentsDestroyed(content::WebContents* web_contents) {
  // TODO(brettw) if a model is attached to the Jaunt associated with the
  // WebContents, I'm not sure about the destruction order and whether this
  // is safe or could cause a use-after-free.
  auto found = tabs_.find(web_contents);
  if (found == tabs_.end())
    return;
  tabs_.erase(found);
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

  scoped_refptr<NavTreePage> new_page =
      base::MakeRefCounted<NavTreePage>(this, url);
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
  // std::ofstream out("P:\\nav_tree.txt", std::ios::out | std::ios::app);
  std::ofstream out("//usr/local/google/home/brettw/nav_tree.txt",
                    std::ios::out | std::ios::app);

  out << "\n==========================\n";
  for (auto& head : tabs_) {
    // Walk backwards to find the root of the tree.
    const NavTreeNode* head_root = head.second.root.get();
    while (head_root->from())
      head_root = head_root->from();

    out << "TAB\n  TREE\n";
    RecursivePrintTree(out, 4, head_root, head_root);
    out << "  JAUNT\n";
    // head.second.root->DebugPrint(out, 4);
  }
}

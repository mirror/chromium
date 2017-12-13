// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/toolbar/jaunt_menu_model.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/history/nav_jaunt.h"
#include "chrome/browser/history/nav_tree.h"
#include "chrome/browser/history/nav_tree_factory.h"
#include "chrome/browser/history/nav_tree_node.h"
#include "chrome/browser/history/nav_tree_page.h"
#include "chrome/browser/history/nav_waypoint.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "content/public/browser/navigation_entry.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

struct JauntMenuModel::FlatMenuEntry {
  bool indented = false;
  gfx::Image icon;
  base::string16 title;
  // Note: don't cache NavWaypoint pointers. They can get destroyed out from
  // under us if the page navigates.
  const NavTreeNode* node = nullptr;
};

JauntMenuModel::JauntMenuModel(Browser* browser)
    : browser_(browser),
      nav_tree_(
          NavTreeFactory::GetForProfile(browser->profile(),
                                        ServiceAccessType::EXPLICIT_ACCESS)) {}

JauntMenuModel::~JauntMenuModel() {}

bool JauntMenuModel::HasIcons() const {
  // return true;  FIXME(brettw)
  return false;
}

int JauntMenuModel::GetItemCount() const {
  const_cast<JauntMenuModel*>(this)->FillCachedItems();
  return static_cast<int>(cached_items_.size());
}

ui::MenuModel::ItemType JauntMenuModel::GetTypeAt(int index) const {
  return TYPE_COMMAND;
}

ui::MenuSeparatorType JauntMenuModel::GetSeparatorTypeAt(int index) const {
  return ui::NORMAL_SEPARATOR;
}

int JauntMenuModel::GetCommandIdAt(int index) const {
  return index;
}

base::string16 JauntMenuModel::GetLabelAt(int index) const {
  if (cached_items_[index].indented)
    return base::ASCIIToUTF16("   ") + cached_items_[index].title;
  return cached_items_[index].title;
}

bool JauntMenuModel::IsItemDynamicAt(int index) const {
  return false;
}

bool JauntMenuModel::GetAcceleratorAt(int index,
                                      ui::Accelerator* accelerator) const {
  return false;
}

bool JauntMenuModel::IsItemCheckedAt(int index) const {
  return false;
}

int JauntMenuModel::GetGroupIdAt(int index) const {
  return 0;
}

bool JauntMenuModel::GetIconAt(int index, gfx::Image* icon) {
  return false;
}

ui::ButtonMenuItemModel* JauntMenuModel::GetButtonMenuItemAt(int index) const {
  return nullptr;
}

bool JauntMenuModel::IsEnabledAt(int index) const {
  return true;
}

ui::MenuModel* JauntMenuModel::GetSubmenuModelAt(int index) const {
  return nullptr;
}

void JauntMenuModel::HighlightChangedTo(int index) {}

void JauntMenuModel::ActivatedAt(int index) {
  ActivatedAt(index, 0);
}

void JauntMenuModel::ActivatedAt(int index, int event_flags) {
  // Called when the user selects a menu item.
  chrome::NavigateToIndexWithDisposition(
      browser_, MenuIndexToNavEntryIndex(index),
      ui::DispositionFromEventFlags(event_flags));
}

void JauntMenuModel::MenuWillShow() {}

void JauntMenuModel::MenuWillClose() {
  cached_items_.clear();
}

void JauntMenuModel::SetMenuModelDelegate(ui::MenuModelDelegate* delegate) {
  menu_model_delegate_ = delegate;
}

ui::MenuModelDelegate* JauntMenuModel::GetMenuModelDelegate() const {
  return menu_model_delegate_;
}

NavJaunt* JauntMenuModel::GetJaunt() {
  return nav_tree_->JauntForWebContents(
      browser_->tab_strip_model()->GetActiveWebContents());
}

void JauntMenuModel::FillCachedItems() {
  cached_items_.clear();
  if (NavJaunt* jaunt = GetJaunt())
    RecursiveAppendCachedItems(jaunt->GetRoot(), false);
}

void JauntMenuModel::RecursiveAppendCachedItems(const NavWaypoint* waypoint,
                                                bool indent) {
  for (int i = 0; i < waypoint->child_count(); i++) {
    const NavWaypoint* cur_child = waypoint->GetChild(i);

    FlatMenuEntry entry;
    entry.indented = indent;
    entry.icon = cur_child->node()->page()->favicon().image;
    entry.title = cur_child->GetTitle();
    entry.node = cur_child->node();
    cached_items_.push_back(std::move(entry));

    if (cur_child->child_count() > 0)
      RecursiveAppendCachedItems(cur_child, true);
  }
}

int JauntMenuModel::MenuIndexToNavEntryIndex(int menu_index) {
  // Correlate the NavTreeNode with a NavigationEntry with the entry ID.
  auto* contents = browser_->tab_strip_model()->GetActiveWebContents();
  auto& nav_controller = contents->GetController();
  int entry_id = cached_items_[menu_index].node->nav_entry_id();

  for (int i = nav_controller.GetEntryCount() - 1; i >= 0; i--) {
    if (nav_controller.GetEntryAtIndex(i)->GetUniqueID() == entry_id)
      return i;
  }
  NOTREACHED();
  return 0;
}

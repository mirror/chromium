// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TOOLBAR_JAUNT_MENU_MODEL_H_
#define CHROME_BROWSER_UI_TOOLBAR_JAUNT_MENU_MODEL_H_

#include "base/macros.h"
#include "ui/base/models/menu_model.h"

class Browser;
class NavJaunt;
class NavTree;
class NavWaypoint;

// Wraps a menu model around a "jaunt" produced by the NavTree. A jaunt
// represents a series of intelligently selected pages from the user's history
// relating to a tab.
class JauntMenuModel : public ui::MenuModel {
 public:
  JauntMenuModel(Browser* browser);
  ~JauntMenuModel() override;

  // MenuModel:
  bool HasIcons() const override;
  int GetItemCount() const override;
  ItemType GetTypeAt(int index) const override;
  ui::MenuSeparatorType GetSeparatorTypeAt(int index) const override;
  int GetCommandIdAt(int index) const override;
  base::string16 GetLabelAt(int index) const override;
  bool IsItemDynamicAt(int index) const override;
  bool GetAcceleratorAt(int index, ui::Accelerator* accelerator) const override;
  bool IsItemCheckedAt(int index) const override;
  int GetGroupIdAt(int index) const override;
  bool GetIconAt(int index, gfx::Image* icon) override;
  ui::ButtonMenuItemModel* GetButtonMenuItemAt(int index) const override;
  bool IsEnabledAt(int index) const override;
  MenuModel* GetSubmenuModelAt(int index) const override;
  void HighlightChangedTo(int index) override;
  void ActivatedAt(int index) override;
  void ActivatedAt(int index, int event_flags) override;
  void MenuWillShow() override;
  void MenuWillClose() override;
  void SetMenuModelDelegate(ui::MenuModelDelegate* delegate) override;
  ui::MenuModelDelegate* GetMenuModelDelegate() const override;

 private:
  struct FlatMenuEntry;

  // Returns the currently active NavJaunt for the current tab in the browser.
  // This should not be cached across function calls since the current tab can
  // change or get destroyed.
  NavJaunt* GetJaunt();

  void FillCachedItems();

  // Appends the children of the given waypoint (recursively) to cached_items_.
  void RecursiveAppendCachedItems(const NavWaypoint* waypoint, bool indent);

  // Returns the navigation entry index in the current tab corresponding to the
  // item at the given menu index.
  int MenuIndexToNavEntryIndex(int menu_index);

  Browser* const browser_;
  NavTree* nav_tree_;

  // When the menu is shown the current tab's Jaunt is flattened to produce
  // this menu model.
  std::vector<FlatMenuEntry> cached_items_;

  ui::MenuModelDelegate* menu_model_delegate_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(JauntMenuModel);
};

#endif  // CHROME_BROWSER_UI_TOOLBAR_JAUNT_MENU_MODEL_H_

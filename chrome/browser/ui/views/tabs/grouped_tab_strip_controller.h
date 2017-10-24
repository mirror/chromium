// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TABS_GROUPED_TAB_STRIP_CONTROLLER_H_
#define CHROME_BROWSER_UI_VIEWS_TABS_GROUPED_TAB_STRIP_CONTROLLER_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/ui/tabs/hover_tab_selector.h"
#include "chrome/browser/ui/tabs/tab_strip_grouper.h"
#include "chrome/browser/ui/tabs/tab_strip_grouper_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/immersive_mode_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "components/prefs/pref_change_registrar.h"

class Browser;
class Tab;
class TabStrip;
struct TabRendererData;

namespace content {
class WebContents;
}

namespace ui {
class ListSelectionModel;
}

class GroupedTabStripController : public TabStripController,
                                  public TabStripGrouperObserver {
 public:
  GroupedTabStripController(TabStripModel* model, BrowserView* browser_view);
  ~GroupedTabStripController() override;

  void InitFromModel(TabStrip* tabstrip);

  TabStripModel* model() const { return model_; }

  bool IsCommandEnabledForTab(TabStripModel::ContextMenuCommand command_id,
                              Tab* tab) const;
  void ExecuteCommandForTab(TabStripModel::ContextMenuCommand command_id,
                            Tab* tab);
  bool IsTabPinned(Tab* tab) const;

  // TabStripController implementation:
  const ui::ListSelectionModel& GetSelectionModel() const override;
  int GetCount() const override;
  bool IsValidIndex(int item_index) const override;
  bool IsActiveTab(int item_index) const override;
  int GetActiveIndex() const override;
  bool IsTabSelected(int item_index) const override;
  bool IsTabPinned(int item_index) const override;
  void SelectTab(int item_index) override;
  void ExtendSelectionTo(int item_index) override;
  void ToggleSelected(int item_index) override;
  void AddSelectionFromAnchorTo(int item_index) override;
  void CloseTab(int item_index, CloseTabSource source) override;
  void ToggleTabAudioMute(int item_index) override;
  void ShowContextMenuForTab(Tab* tab,
                             const gfx::Point& p,
                             ui::MenuSourceType source_type) override;
  void UpdateLoadingAnimations() override;
  int HasAvailableDragActions() const override;
  void OnDropIndexUpdate(int item_index, bool drop_before) override;
  void PerformDrop(bool drop_before, int item_index, const GURL& url) override;
  bool IsCompatibleWith(TabStrip* other) const override;
  void CreateNewTab() override;
  void CreateNewTabWithLocation(const base::string16& loc) override;
  bool IsIncognito() override;
  void StackedLayoutMaybeChanged() override;
  void OnStartedDraggingTabs() override;
  void OnStoppedDraggingTabs() override;
  void CheckFileSupported(const GURL& url) override;
  SkColor GetToolbarTopSeparatorColor() const override;
  base::string16 GetAccessibleTabName(const Tab* tab) const override;
  Profile* GetProfile() const override;

  // TabStripGrouper implementation:
  void ItemsChanged(const std::vector<int>& removed,
                    const std::vector<int>& added) override;
  void ItemUpdatedAt(int item_index, TabChangeType change_type) override;
  void ItemSelectionChanged(const ui::ListSelectionModel& old_model) override;
  void ItemNeedsAttentionAt(int index) override;

  const Browser* browser() const { return browser_view_->browser(); }

 protected:
  // The context in which SetTabRendererDataFromModel is being called.
  enum TabStatus { NEW_TAB, EXISTING_TAB };

  // Sets the TabRendererData from the TabStripModel.
  virtual void SetTabRendererDataFromModel(content::WebContents* contents,
                                           int model_index,
                                           TabRendererData* data,
                                           TabStatus tab_status);
  virtual std::vector<TabRendererData> GetTabRendererDataFromGrouper(
      TabStripGrouperItem* item,
      TabStatus tab_status);

  const TabStrip* tabstrip() const { return tabstrip_; }

 private:
  class TabContextMenuContents;

  void SetItemDataAt(int item_index);

  void StartHighlightTabsForCommand(
      TabStripModel::ContextMenuCommand command_id,
      Tab* tab);
  void StopHighlightTabsForCommand(TabStripModel::ContextMenuCommand command_id,
                                   Tab* tab);

  // Adds a tab.
  void AddTab(int item_index, bool is_active);

  // Resets the tabstrips stacked layout (true or false) from prefs.
  void UpdateStackedLayout();

  // Notifies the tabstrip whether |url| is supported once a MIME type request
  // has completed.
  void OnFindURLMimeTypeCompleted(const GURL& url,
                                  const std::string& mime_type);

  TabStripModel* model_;
  TabStripGrouper grouper_;

  TabStrip* tabstrip_;

  BrowserView* browser_view_;

  // If non-NULL it means we're showing a menu for the tab.
  std::unique_ptr<TabContextMenuContents> context_menu_contents_;

  // Helper for performing tab selection as a result of dragging over a tab.
  HoverTabSelector hover_tab_selector_;

  // Forces the tabs to use the regular (non-immersive) style and the
  // top-of-window views to be revealed when the user is dragging |tabstrip|'s
  // tabs.
  std::unique_ptr<ImmersiveRevealedLock> immersive_reveal_lock_;

  PrefChangeRegistrar local_pref_registrar_;

  base::WeakPtrFactory<GroupedTabStripController> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(GroupedTabStripController);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TABS_GROUPED_TAB_STRIP_CONTROLLER_H_

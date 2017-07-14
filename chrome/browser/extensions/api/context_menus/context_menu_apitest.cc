// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/extensions/extension_action.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/menu_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/extensions/api/context_menus.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/context_menu_params.h"
#include "extensions/test/result_catcher.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/models/menu_model.h"

namespace {

// Returns the active WebContents.
content::WebContents* GetWebContents(const Browser* browser) {
  return browser->tab_strip_model()->GetActiveWebContents();
}

}  // namespace

namespace extensions {

class ExtensionContextMenuApiTest : public ExtensionApiTest {
 public:
  MenuItem* GetMenuItem(const std::unique_ptr<TestRenderViewContextMenu>& menu,
                        int commandId) {
    return menu->extension_items().GetExtensionMenuItem(commandId);
  }

  int CountItemsInModel(const ui::MenuModel* model) {
    int num_items_found = 0;
    if (model) {
      for (int i = 0; i < model->GetItemCount(); ++i)
        ++num_items_found;
    }
    return num_items_found;
  }

  int CountItemsInMenu(const std::unique_ptr<TestRenderViewContextMenu>& menu) {
    return menu->extension_items().extension_item_map_.size();
  }

  int ConvertToExtensionsCustomCommandId(
      const std::unique_ptr<TestRenderViewContextMenu>& menu,
      int id) {
    return menu.get()
               ? menu->extension_items().ConvertToExtensionsCustomCommandId(id)
               : -1;
  }

  bool GetTopLevelMenuModel(Browser* browser,
                            std::unique_ptr<TestRenderViewContextMenu>& menu,
                            ui::MenuModel** model,
                            int* index) {
    content::RenderFrameHost* frame = GetWebContents(browser)->GetMainFrame();
    content::ContextMenuParams params;
    params.page_url = frame->GetLastCommittedURL();

    // Create context menu.
    menu.reset(new TestRenderViewContextMenu(frame, params));
    menu->Init();

    // Get context menu.
    return menu->GetMenuModelAndItemIndex(
        ConvertToExtensionsCustomCommandId(menu, 0), model, index);
  }
};

// Times out on win syzyasan, http://crbug.com/166026
#if defined(SYZYASAN)
#define MAYBE_ContextMenus DISABLED_ContextMenus
#else
#define MAYBE_ContextMenus ContextMenus
#endif
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, MAYBE_ContextMenus) {
  ASSERT_TRUE(RunExtensionTest("context_menus/basics")) << message_;
  ASSERT_TRUE(RunExtensionTest("context_menus/no_perms")) << message_;
  ASSERT_TRUE(RunExtensionTest("context_menus/item_ids")) << message_;
  ASSERT_TRUE(RunExtensionTest("context_menus/event_page")) << message_;
}

// crbug.com/51436 -- creating context menus from multiple script contexts
// should work.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, ContextMenusFromMultipleContexts) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(RunExtensionTest("context_menus/add_from_multiple_contexts"))
      << message_;
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  {
    // Tell the extension to update the page action state.
    ResultCatcher catcher;
    ui_test_utils::NavigateToURL(browser(),
        extension->GetResourceURL("popup.html"));
    ASSERT_TRUE(catcher.GetNextResult());
  }

  {
    // Tell the extension to update the page action state again.
    ResultCatcher catcher;
    ui_test_utils::NavigateToURL(browser(),
        extension->GetResourceURL("popup2.html"));
    ASSERT_TRUE(catcher.GetNextResult());
  }
}

// Tests adding a single visible menu item to the top-level menu model, which
// includes actions like "Back", "View Page Source", "Inspect", etc.
IN_PROC_BROWSER_TEST_F(ExtensionContextMenuApiTest,
                       ContextMenusCreateOneVisibleTopLevelItem) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(
      RunExtensionTest("context_menus/item_visibility/one_top_level_menu_item/"
                       "visible_item_no_children"))
      << message_;
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Get context menu.
  //
  // Note that |top_level_model| refers to the list of menu items (both related
  // and unrelated to extensions) that is passed to UI code to be displayed.
  // Because the current UI code does not check for item visibility,
  // |top_level_model| does not include hidden items (TODO: crbug.com/). |menu|
  // includes only extension menu items, regardless of visibility.
  std::unique_ptr<TestRenderViewContextMenu> menu;
  ui::MenuModel* top_level_model = nullptr;
  int top_level_index = -1;
  ASSERT_TRUE(GetTopLevelMenuModel(browser(), menu, &top_level_model,
                                   &top_level_index));

  ASSERT_TRUE(top_level_model);
  ASSERT_TRUE(top_level_index != -1);

  // The top-level context menu model has 12 items, e.g. "View Page Source",
  // "Inspect", etc. Since the extension is adding one context menu item to the
  // top-level, there will be 12 + 1 = 13 menu items.
  EXPECT_EQ(13, CountItemsInModel(top_level_model));

  // There should only be one item in the extension context |menu|.
  ASSERT_TRUE(menu.get());
  EXPECT_EQ(1, CountItemsInMenu(menu));

  // Verify the visibility of the item.
  EXPECT_TRUE(top_level_model->IsVisibleAt(top_level_index));
  EXPECT_EQ(base::UTF8ToUTF16("item"),
            top_level_model->GetLabelAt(top_level_index));
  ASSERT_EQ(ui::MenuModel::TYPE_COMMAND,
            top_level_model->GetTypeAt(top_level_index));

  // There should be no submenu model.
  ui::MenuModel* submodel = top_level_model->GetSubmenuModelAt(top_level_index);
  ASSERT_TRUE(submodel == NULL);
}

// Tests that a hidden menu item should not be added to the top-level menu
// model, which includes actions like "Back", "View Page Source", "Inspect",
// etc.
IN_PROC_BROWSER_TEST_F(ExtensionContextMenuApiTest,
                       ContextMenusDoNotCreateTopLevelItemIfHidden) {
  ASSERT_TRUE(embedded_test_server()->Start());
  // TODO: Come up with better name for this file folder.
  ASSERT_TRUE(
      RunExtensionTest("context_menus/item_visibility/one_top_level_menu_item/"
                       "hidden_item_no_children"))
      << message_;
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Get context menu.
  //
  // Note that |top_level_model| refers to the list of menu items (both related
  // and unrelated to extensions) that is passed to UI code to be displayed.
  // Because the current UI code does not check for item visibility,
  // |top_level_model| does not include hidden items (TODO: crbug.com/). |menu|
  // includes only extension menu items, regardless of visibility.
  std::unique_ptr<TestRenderViewContextMenu> menu;
  ui::MenuModel* top_level_model = nullptr;
  int top_level_index = -1;

  // There should be no extension model found.
  ASSERT_FALSE(GetTopLevelMenuModel(browser(), menu, &top_level_model,
                                    &top_level_index));
  ASSERT_FALSE(top_level_model);
  ASSERT_TRUE(top_level_index == -1);

  // There should be no extension items in the menu model.
  EXPECT_EQ(0, CountItemsInModel(top_level_model));

  // The hidden menu item should still be stored in |menu|.
  ASSERT_TRUE(menu.get());
  EXPECT_EQ(1, CountItemsInMenu(menu));

  // Check that the item created is hidden.
  int menu_index = 0;
  int id = ConvertToExtensionsCustomCommandId(menu, menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(id));
  EXPECT_EQ("item", GetMenuItem(menu, id)->title());
}

// Tests that if a parent menu item and its children are both hidden, then the
// parent item should not be added to the top-level menu model, which includes
// actions like "Back", "View Page Source", "Inspect", etc. This also tests that
// no submenu model is created for the hidden children.
IN_PROC_BROWSER_TEST_F(
    ExtensionContextMenuApiTest,
    ContextMenusDoNotCreateTopLevelSubmenuItemIfHiddenAndChildrenHidden) {
  ASSERT_TRUE(embedded_test_server()->Start());
  // TODO: Come up with better name for this file folder.
  ASSERT_TRUE(
      RunExtensionTest("context_menus/item_visibility/one_top_level_menu_item/"
                       "hidden_parent_and_children"))
      << message_;
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Get context menu.
  //
  // Note that |top_level_model| refers to the list of menu items (both related
  // and unrelated to extensions) that is passed to UI code to be displayed.
  // Because the current UI code does not check for item visibility,
  // |top_level_model| does not include hidden items (TODO: crbug.com/). |menu|
  // includes only extension menu items, regardless of visibility.
  std::unique_ptr<TestRenderViewContextMenu> menu;
  ui::MenuModel* top_level_model = nullptr;
  int top_level_index = -1;

  // There should be no extension model found.
  ASSERT_FALSE(GetTopLevelMenuModel(browser(), menu, &top_level_model,
                                    &top_level_index));
  ASSERT_FALSE(top_level_model);
  ASSERT_TRUE(top_level_index == -1);

  // There should be no extension items in the menu model.
  EXPECT_EQ(0, CountItemsInModel(top_level_model));

  // The hidden menu item and its two hidden children should still be stored in
  // |menu|.
  ASSERT_TRUE(menu.get());
  EXPECT_EQ(3, CountItemsInMenu(menu));

  // Check that the parent item is hidden.
  int menu_index = 0;
  int parent_id = ConvertToExtensionsCustomCommandId(menu, menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(parent_id));
  EXPECT_EQ("parent", GetMenuItem(menu, parent_id)->title());

  // Check that the two children should be hidden.
  int child1_id = ConvertToExtensionsCustomCommandId(menu, ++menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(child1_id));
  EXPECT_EQ("child1", GetMenuItem(menu, child1_id)->title());

  int child2_id = ConvertToExtensionsCustomCommandId(menu, ++menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(child2_id));
  EXPECT_EQ("child2", GetMenuItem(menu, child2_id)->title());
}

// Tests that if a parent menu item is hidden and some of its children are
// visible, then the parent item should not be added to the top-level menu
// model, which includes actions like "Back", "View Page Source", "Inspect",
// etc. This also tests that no submenu model is created for the children.
IN_PROC_BROWSER_TEST_F(
    ExtensionContextMenuApiTest,
    ContextMenusDoNotCreateTopLevelSubmenuItemIfHiddenAndSomeChildrenVisible) {
  ASSERT_TRUE(embedded_test_server()->Start());
  // TODO: Come up with better name for this file folder.
  ASSERT_TRUE(
      RunExtensionTest("context_menus/item_visibility/one_top_level_menu_item/"
                       "hidden_parent_and_some_visible_children"))
      << message_;
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Get context menu.
  //
  // Note that |top_level_model| refers to the list of menu items (both related
  // and unrelated to extensions) that is passed to UI code to be displayed.
  // Because the current UI code does not check for item visibility,
  // |top_level_model| does not include hidden items (TODO: crbug.com/). |menu|
  // includes only extension menu items, regardless of visibility.
  std::unique_ptr<TestRenderViewContextMenu> menu;
  ui::MenuModel* top_level_model = nullptr;
  int top_level_index = -1;

  // There should be no extension model found.
  ASSERT_FALSE(GetTopLevelMenuModel(browser(), menu, &top_level_model,
                                    &top_level_index));
  ASSERT_FALSE(top_level_model);
  ASSERT_TRUE(top_level_index == -1);

  // There should be no extension items in the menu model.
  EXPECT_EQ(0, CountItemsInModel(top_level_model));

  // There should be four items in |menu|: the hiden top-level parent item and
  // its three children (hidden, visible, hidden).
  ASSERT_TRUE(menu.get());
  EXPECT_EQ(4, CountItemsInMenu(menu));

  // Check that the parent item is hidden.
  int menu_index = 0;
  int parent_id = ConvertToExtensionsCustomCommandId(menu, menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(parent_id));
  EXPECT_EQ("parent", GetMenuItem(menu, parent_id)->title());

  // Check that the three children should be hidden.
  int child1_id = ConvertToExtensionsCustomCommandId(menu, ++menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(child1_id));
  EXPECT_EQ("child1", GetMenuItem(menu, child1_id)->title());

  int child2_id = ConvertToExtensionsCustomCommandId(menu, ++menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(child2_id));
  EXPECT_EQ("child2", GetMenuItem(menu, child2_id)->title());

  int child3_id = ConvertToExtensionsCustomCommandId(menu, ++menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(child3_id));
  EXPECT_EQ("child3", GetMenuItem(menu, child3_id)->title());
}

// Tests that a single top-level parent menu item should be created when all of
// its child items are hidden. The child items' hidden states are tested too.
// Recall that a top-level item can be either a parent item specified by the
// developer or parent item labeled with the extension's name. In this case, we
// test the former.
IN_PROC_BROWSER_TEST_F(
    ExtensionContextMenuApiTest,
    ContextMenusCreateTopLevelItemIfAllItsChildrenAreHidden) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(
      RunExtensionTest("context_menus/item_visibility/one_top_level_menu_item/"
                       "visible_parent_and_hidden_children"))
      << message_;
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Get context menu.
  //
  // Note that |top_level_model| refers to the list of menu items (both related
  // and unrelated to extensions) that is passed to UI code to be displayed.
  // Because the current UI code does not check for item visibility,
  // |top_level_model| does not include hidden items (TODO: crbug.com/). |menu|
  // includes only extension menu items, regardless of visibility.
  std::unique_ptr<TestRenderViewContextMenu> menu;
  ui::MenuModel* top_level_model = nullptr;
  int top_level_index = -1;
  ASSERT_TRUE(GetTopLevelMenuModel(browser(), menu, &top_level_model,
                                   &top_level_index));

  ASSERT_TRUE(top_level_model);
  ASSERT_TRUE(top_level_index != -1);

  // There should be three items in |menu|: the top-level parent items and its
  // two hidden children.
  ASSERT_TRUE(menu.get());
  EXPECT_EQ(3, CountItemsInMenu(menu));

  // The top-level context menu model has 12 items, e.g. "View Page Source",
  // "Inspect", etc. Since the extension is adding one context menu item to the
  // top-level, there will be 12 + 1 = 13 menu items.
  EXPECT_EQ(13, CountItemsInModel(top_level_model));

  // Verify the parent item is visible in the top-level menu model.
  EXPECT_TRUE(top_level_model->IsVisibleAt(top_level_index));
  EXPECT_EQ(base::UTF8ToUTF16("parent"),
            top_level_model->GetLabelAt(top_level_index));
  ASSERT_EQ(ui::MenuModel::TYPE_COMMAND,
            top_level_model->GetTypeAt(top_level_index));

  // There should be no submenu model, since the chidlren are hidden.
  ui::MenuModel* submodel = top_level_model->GetSubmenuModelAt(top_level_index);
  ASSERT_TRUE(submodel == NULL);

  // Check that the two children item are hidden.
  //
  // The first item starts at menu_index = 1 because the zero index is reserved
  // for the top-level item in the model.
  int menu_index = 1;
  int child1_id = ConvertToExtensionsCustomCommandId(menu, menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(child1_id));
  EXPECT_EQ("child1", GetMenuItem(menu, child1_id)->title());

  int child2_id = ConvertToExtensionsCustomCommandId(menu, ++menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(child2_id));
  EXPECT_EQ("child2", GetMenuItem(menu, child2_id)->title());
}

// Tests that a top-level parent menu item should be created as a submenu,
// when some of its child items are visibile. The child items' visibilities are
// tested too. Recall that a top-level item can be either a parent item
// specified by the developer or parent item labeled with the extension's name.
// In this case, we test the former.
IN_PROC_BROWSER_TEST_F(
    ExtensionContextMenuApiTest,
    ContextMenusCreateSubmenuTopLevelItemIfSomeOfChildrenAreVisible) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(
      RunExtensionTest("context_menus/item_visibility/one_top_level_menu_item/"
                       "visible_parent_and_some_visible_children"))
      << message_;
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Get context menu.
  //
  // Note that |top_level_model| refers to the list of menu items (both related
  // and unrelated to extensions) that is passed to UI code to be displayed.
  // Because the current UI code does not check for item visibility,
  // |top_level_model| does not include hidden items (TODO: crbug.com/). |menu|
  // includes only extension menu items, regardless of visibility.
  std::unique_ptr<TestRenderViewContextMenu> menu;
  ui::MenuModel* top_level_model = nullptr;
  int top_level_index = -1;
  ASSERT_TRUE(GetTopLevelMenuModel(browser(), menu, &top_level_model,
                                   &top_level_index));

  ASSERT_TRUE(top_level_model);
  ASSERT_TRUE(top_level_index != -1);

  // There should be four items in |menu|: the top-level parent item and its
  // three children (hidden, visible, hidden).
  ASSERT_TRUE(menu.get());
  EXPECT_EQ(4, CountItemsInMenu(menu));

  // The top-level context menu model has 12 items, e.g. "View Page Source",
  // "Inspect", etc. Since the extension is adding one context menu item to the
  // top-level, there will be 12 + 1 = 13 menu items.
  EXPECT_EQ(13, CountItemsInModel(top_level_model));

  // Verify the parent item is visible in the top-level menu model.
  EXPECT_TRUE(top_level_model->IsVisibleAt(top_level_index));
  EXPECT_EQ(base::UTF8ToUTF16("parent"),
            top_level_model->GetLabelAt(top_level_index));

  // The top-level item should be a submenu, since some of the children are
  // visible.
  ASSERT_EQ(ui::MenuModel::TYPE_SUBMENU,
            top_level_model->GetTypeAt(top_level_index));
  ui::MenuModel* submodel = top_level_model->GetSubmenuModelAt(top_level_index);
  ASSERT_TRUE(submodel != NULL);
  // Check that the submodel should only have 1 item - the visible second child.
  EXPECT_EQ(1, CountItemsInModel(submodel));

  // Check that the first child item is hidden.
  //
  // The first item starts at menu_index = 1 because the zero index is reserved
  // for the top-level item in the model.
  int menu_index = 1;
  int child1_id = ConvertToExtensionsCustomCommandId(menu, menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(child1_id));
  EXPECT_EQ("child1", GetMenuItem(menu, child1_id)->title());

  // Check that the second child is visible and included in the |submodel|.
  int subindex = 0;
  EXPECT_TRUE(submodel->IsVisibleAt(subindex));
  EXPECT_EQ(base::ASCIIToUTF16("child2"), submodel->GetLabelAt(subindex));

  // Skip the second child item in the menu.
  ++menu_index;

  // Check that the third child item is hidden.
  int child3_id = ConvertToExtensionsCustomCommandId(menu, ++menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(child3_id));
  EXPECT_EQ("child3", GetMenuItem(menu, child3_id)->title());
}

// Tests that no top-level parent menu item should be created when all of its
// child items are hidden. Recall that a top-level item can be either a parent
// item specified by the developer or parent item labeled with the extension's
// name. In this case, we test the latter.
IN_PROC_BROWSER_TEST_F(ExtensionContextMenuApiTest,
                       ContextMenusCreateAllHiddenItems) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(
      RunExtensionTest("context_menus/item_visibility/"
                       "extension_items_container/all_items_hidden"))
      << message_;
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Get context menu.
  //
  // Note that |top_level_model| refers to the list of menu items (both related
  // and unrelated to extensions) that is passed to UI code to be displayed.
  // Because the current UI code does not check for item visibility,
  // |top_level_model| does not include hidden items (TODO: crbug.com/). |menu|
  // includes only extension menu items, regardless of visibility.
  std::unique_ptr<TestRenderViewContextMenu> menu;
  ui::MenuModel* top_level_model = nullptr;
  int top_level_index = -1;

  // There should be no extension model found.
  ASSERT_FALSE(GetTopLevelMenuModel(browser(), menu, &top_level_model,
                                    &top_level_index));
  ASSERT_FALSE(top_level_model);
  ASSERT_TRUE(top_level_index == -1);

  // The hidden menu items should still be stored in |menu|.
  ASSERT_TRUE(menu.get());
  EXPECT_EQ(3, CountItemsInMenu(menu));

  // The first item starts at menu_index = 1 because the zero index is reserved
  // for the top-level item in the model, even if the top-level item is not
  // visible like in this case.
  int menu_index = 1;
  int item1_id = ConvertToExtensionsCustomCommandId(menu, menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(item1_id));
  EXPECT_EQ("item1", GetMenuItem(menu, item1_id)->title());

  int item2_id = ConvertToExtensionsCustomCommandId(menu, ++menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(item2_id));
  EXPECT_EQ("item2", GetMenuItem(menu, item2_id)->title());

  int item3_id = ConvertToExtensionsCustomCommandId(menu, ++menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(item3_id));
  EXPECT_EQ("item3", GetMenuItem(menu, item3_id)->title());
}

// Tests that a top-level parent menu item should be created when some of its
// child items are visible. The child items' visibilities are tested as well.
// Recall that a top-level item can be either a parent item specified by the
// developer or parent item labeled with the extension's name. In this case, we
// test the latter.
//
// Also, this tests that hiding a parent item should hide its children even if
// they are set as visible.
IN_PROC_BROWSER_TEST_F(ExtensionContextMenuApiTest,
                       ContextMenusCreateWithCustomVisibility) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(RunExtensionTest(
      "context_menus/item_visibility/extension_items_container/"
      "visible_parent_and_some_visible_children"))
      << message_;
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Get context menu.
  //
  // Note that |top_level_model| refers to the list of menu items (both related
  // and unrelated to extensions) that is passed to UI code to be displayed.
  // Because the current UI code does not check for item visibility,
  // |top_level_model| does not include hidden items (TODO: crbug.com/). |menu|
  // includes only extension menu items, regardless of visibility.
  std::unique_ptr<TestRenderViewContextMenu> menu;
  ui::MenuModel* top_level_model = nullptr;
  int top_level_index = -1;
  ASSERT_TRUE(GetTopLevelMenuModel(browser(), menu, &top_level_model,
                                   &top_level_index));

  ASSERT_TRUE(top_level_model);
  ASSERT_TRUE(top_level_index != -1);

  // The top-level context menu model has 12 items, e.g. "View Page Source",
  // "Inspect", etc. Since the extension is adding one context menu item to the
  // top-level, there will be 12 + 1 = 13 menu items.
  EXPECT_EQ(13, CountItemsInModel(top_level_model));

  // There should be a total of five menu items in the |menu|. The first two are
  // visible. Note that the last three hidden items are still stored in |menu|.
  ASSERT_TRUE(menu.get());
  EXPECT_EQ(5, CountItemsInMenu(menu));

  // The top-level item in the model should be an "automagic parent" with the
  // extension's name.
  EXPECT_EQ(base::UTF8ToUTF16(extension->name()),
            top_level_model->GetLabelAt(top_level_index));
  ASSERT_EQ(ui::MenuModel::TYPE_SUBMENU,
            top_level_model->GetTypeAt(top_level_index));

  // Get the submenu and verify the items there.
  ui::MenuModel* submodel = top_level_model->GetSubmenuModelAt(top_level_index);
  ASSERT_TRUE(submodel != NULL);
  int subindex = 0;
  // There should be two menu items present in the |submodel|. The others should
  // be hidden and not included in |submodel|.
  EXPECT_EQ(2, CountItemsInModel(submodel));

  EXPECT_TRUE(submodel->IsVisibleAt(subindex));
  EXPECT_EQ(base::ASCIIToUTF16("item1"), submodel->GetLabelAt(subindex));

  EXPECT_TRUE(submodel->IsVisibleAt(++subindex));
  EXPECT_EQ(base::ASCIIToUTF16("item2"), submodel->GetLabelAt(subindex));

  int menu_index = subindex + 1;
  // The third menu item and its children should be hidden.
  int item3_id = ConvertToExtensionsCustomCommandId(menu, ++menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(item3_id));
  EXPECT_EQ("item3", GetMenuItem(menu, item3_id)->title());

  int child1_id = ConvertToExtensionsCustomCommandId(menu, ++menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(child1_id));
  EXPECT_EQ("child1", GetMenuItem(menu, child1_id)->title());

  int child2_id = ConvertToExtensionsCustomCommandId(menu, ++menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(child2_id));
  EXPECT_EQ("child2", GetMenuItem(menu, child2_id)->title());
}

// Tests |visible| field in updateProperties (chrome.contextMenus.update).
IN_PROC_BROWSER_TEST_F(ExtensionContextMenuApiTest,
                       ContextMenusUpdateItemVisibility) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(RunExtensionTest(
      "context_menus/item_visibility/extension_items_container/update_item"))
      << message_;
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Get context menu.
  std::unique_ptr<TestRenderViewContextMenu> menu;
  ui::MenuModel* top_level_model = nullptr;
  int top_level_index = -1;
  ASSERT_TRUE(GetTopLevelMenuModel(browser(), menu, &top_level_model,
                                   &top_level_index));
  ASSERT_TRUE(top_level_model);
  ASSERT_TRUE(top_level_index != -1);
  ASSERT_TRUE(menu.get());

  // The top-level context menu top_level_model has 12 items, e.g. "View Page
  // Source", "Inspect", etc. Since the extension is adding one context menu
  // item to the top-level, there will be 12 + 1 = 13 menu items.
  EXPECT_EQ(13, CountItemsInModel(top_level_model));

  // The top-level item should be an "automagic parent" with the extension's
  // name.
  EXPECT_EQ(base::UTF8ToUTF16(extension->name()),
            top_level_model->GetLabelAt(top_level_index));
  ASSERT_EQ(ui::MenuModel::TYPE_SUBMENU,
            top_level_model->GetTypeAt(top_level_index));

  // Get the submenu and verify the items there.
  ui::MenuModel* submodel = top_level_model->GetSubmenuModelAt(top_level_index);
  ASSERT_TRUE(submodel != NULL);
  int submodel_index = 0;
  // There should be three menu items present.
  EXPECT_EQ(3, CountItemsInModel(submodel));

  // The first two items should be visible.
  EXPECT_TRUE(submodel->IsVisibleAt(submodel_index));
  EXPECT_EQ(base::ASCIIToUTF16("item1"), submodel->GetLabelAt(submodel_index));

  EXPECT_TRUE(submodel->IsVisibleAt(++submodel_index));
  EXPECT_EQ(base::ASCIIToUTF16("item2"), submodel->GetLabelAt(submodel_index));

  // The third item is a parent submenu with two child items.
  EXPECT_TRUE(submodel->IsVisibleAt(++submodel_index));
  EXPECT_EQ(base::ASCIIToUTF16("item3"), submodel->GetLabelAt(submodel_index));
  ASSERT_EQ(ui::MenuModel::TYPE_SUBMENU, submodel->GetTypeAt(submodel_index));

  ui::MenuModel* parent_submodel = submodel->GetSubmenuModelAt(submodel_index);
  // There should be 1 child menu item present. The other child is hidden.
  EXPECT_EQ(1, CountItemsInModel(parent_submodel));
  int parent_submodel_index = 0;
  EXPECT_TRUE(parent_submodel->IsVisibleAt(parent_submodel_index));
  EXPECT_EQ(base::ASCIIToUTF16("child1"),
            parent_submodel->GetLabelAt(parent_submodel_index));

  // The second child should be hidden. Note that because it is not included in
  // the menu top_level_model, we do not use the top_level_model->GetCommandIdAt
  // accessor to convert the command id to an extension custom id. Also note,
  // there should be five items in the menu (RenderViewContextMenu). Four are
  // visible, and the fifth is hidden; hence, the use of top_level_index 5
  // below.
  int menu_index = submodel_index + 1;
  // Skip the first child.
  ++menu_index;
  int child2_id = ConvertToExtensionsCustomCommandId(menu, ++menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(child2_id));
  EXPECT_EQ("child2", GetMenuItem(menu, child2_id)->title());
}

// Tests that hidden items are not counted against
// chrome.contextMenus.ACTION_MENU_TOP_LEVEL_LIMIT.
IN_PROC_BROWSER_TEST_F(ExtensionContextMenuApiTest,
                       ContextMenusItemLimitVisibility) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(RunExtensionTest(
      "context_menus/item_visibility/extension_items_container/item_limit"))
      << message_;
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Get context menu.
  std::unique_ptr<TestRenderViewContextMenu> menu;
  ui::MenuModel* top_level_model = nullptr;
  int top_level_index = -1;
  ASSERT_TRUE(GetTopLevelMenuModel(browser(), menu, &top_level_model,
                                   &top_level_index));
  ASSERT_TRUE(top_level_model);
  ASSERT_TRUE(top_level_index != -1);
  ASSERT_TRUE(menu.get());

  // The top-level context menu top_level_model has 12 items, e.g. "View Page
  // Source", "Inspect", etc. Since the extension is adding one context menu
  // item to the top-level, there will be 12 + 1 = 13 menu items.
  EXPECT_EQ(13, CountItemsInModel(top_level_model));

  // The top-level item should be an "automagic parent" with the extension's
  // name.
  EXPECT_EQ(base::UTF8ToUTF16(extension->name()),
            top_level_model->GetLabelAt(top_level_index));
  ASSERT_EQ(ui::MenuModel::TYPE_SUBMENU,
            top_level_model->GetTypeAt(top_level_index));

  // Get the submenu and verify the items there.
  ui::MenuModel* submodel = top_level_model->GetSubmenuModelAt(top_level_index);
  ASSERT_TRUE(submodel != NULL);
  int submodel_index = 0;

  // The top_level_model should show six items (ACTION_MENU_TOP_LEVEL_LIMIT =
  // 6).
  EXPECT_EQ(api::context_menus::ACTION_MENU_TOP_LEVEL_LIMIT,
            CountItemsInModel(submodel));

  // The first two items created should be hidden.
  //
  // Note that they will not be included in the menu top_level_model, so we do
  // not use the top_level_model->GetCommandIdAt accessor to convert the command
  // id to an extension custom id.
  //
  // Also note that because the visibility of the items was toggled via a
  // chrome.contextMenus.update(), the items will be at the end of the menu
  // list. There should be eight items in the menu (RenderViewContextMenu);
  // hence, the use of indicies 7 and 8 below.
  int menu_index = 0;
  int item1_id = ConvertToExtensionsCustomCommandId(menu, ++menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(item1_id));
  EXPECT_EQ("item1", GetMenuItem(menu, item1_id)->title());

  int item2_id = ConvertToExtensionsCustomCommandId(menu, ++menu_index);
  EXPECT_FALSE(menu->IsCommandIdVisible(item2_id));
  EXPECT_EQ("item2", GetMenuItem(menu, item2_id)->title());

  // The next four items created should be visisble.
  EXPECT_TRUE(submodel->IsVisibleAt(submodel_index));
  EXPECT_EQ(base::ASCIIToUTF16("item3"), submodel->GetLabelAt(submodel_index));

  EXPECT_TRUE(submodel->IsVisibleAt(++submodel_index));
  EXPECT_EQ(base::ASCIIToUTF16("item4"), submodel->GetLabelAt(submodel_index));

  EXPECT_TRUE(submodel->IsVisibleAt(++submodel_index));
  EXPECT_EQ(base::ASCIIToUTF16("item5"), submodel->GetLabelAt(submodel_index));

  EXPECT_TRUE(submodel->IsVisibleAt(++submodel_index));
  EXPECT_EQ(base::ASCIIToUTF16("item6"), submodel->GetLabelAt(submodel_index));

  // The last two items that were added in place of the first two items should
  // be visible.
  EXPECT_TRUE(submodel->IsVisibleAt(++submodel_index));
  EXPECT_EQ(base::ASCIIToUTF16("newItem1"),
            submodel->GetLabelAt(submodel_index));

  EXPECT_TRUE(submodel->IsVisibleAt(++submodel_index));
  EXPECT_EQ(base::ASCIIToUTF16("newItem2"),
            submodel->GetLabelAt(submodel_index));
}

}  // namespace extensions

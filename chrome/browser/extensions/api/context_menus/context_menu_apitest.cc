// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/path_service.h"
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
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/api/context_menus.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/context_menu_params.h"
#include "content/public/test/browser_test_utils.h"
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
  ExtensionContextMenuApiTest()
      : top_level_model_(nullptr),
        menu_(nullptr),
        top_level_index_(new int(-1)) {}

  void SetUpTestExtension() {
    PathService::Get(chrome::DIR_TEST_DATA, &test_data_dir_);
    test_data_dir_ = test_data_dir_.AppendASCII(
        "extensions/api_test/context_menus/item_visibility/");
    extension_ = LoadExtension(test_data_dir_);
  }

  int CountItemsInMenu(TestRenderViewContextMenu* menu) {
    return menu->extension_items().extension_item_map_.size();
  }

  int ConvertToExtensionsCustomCommandId(
      const std::unique_ptr<TestRenderViewContextMenu>& menu,
      int id) {
    return menu.get()
               ? menu->extension_items().ConvertToExtensionsCustomCommandId(id)
               : -1;
  }

  // Sets up the top-level model, which is the list of menu items (both related
  // and unrelated to extensions) that is passed to UI code to be displayed.
  bool SetupTopLevelMenuModel() {
    content::RenderFrameHost* frame = GetWebContents(browser())->GetMainFrame();
    content::ContextMenuParams params;
    params.page_url = frame->GetLastCommittedURL();

    // Create context menu.
    menu_.reset(new TestRenderViewContextMenu(frame, params));
    menu_->Init();

    // Get menu model.
    bool valid_setup = menu_->GetMenuModelAndItemIndex(
        ConvertToExtensionsCustomCommandId(menu_, 0), &top_level_model_,
        top_level_index_.get());

    EXPECT_GT(GetTopLevelIndex(), 0);

    return valid_setup;
  }

  void CallAPI(const std::string& script) {
    content::RenderViewHost* background_page =
        GetBackgroundPage(extension_->id());
    bool success = false;
    ASSERT_TRUE(content::ExecuteScriptAndExtractBool(background_page, script,
                                                     &success));
    ASSERT_FALSE(success);
  }

  // Verifies that the context menu is valid and contains the given number of
  // menu items, |numItems|.
  void VerifyNumContextMenuItems(int numItems) {
    ASSERT_TRUE(GetMenu());
    EXPECT_EQ(numItems, CountItemsInMenu(GetMenu()));
  }

  // Verifies a context menu item's visibility, title, and item type.
  void VerifyMenuItem(const std::string& title,
                      ui::MenuModel* model,
                      int index,
                      ui::MenuModel::ItemType type,
                      bool visible) {
    EXPECT_EQ(base::ASCIIToUTF16(title), model->GetLabelAt(index));
    ASSERT_EQ(type, model->GetTypeAt(index));
    EXPECT_EQ(visible, model->IsVisibleAt(index));
  }

  int GetTopLevelIndex() { return *top_level_index_; }

  TestRenderViewContextMenu* GetMenu() { return menu_.get(); }

  const extensions::Extension* extension() { return extension_; }

  ui::MenuModel* top_level_model_;

 private:
  content::RenderViewHost* GetBackgroundPage(const std::string& extension_id) {
    return process_manager()
        ->GetBackgroundHostForExtension(extension_id)
        ->render_view_host();
  }

  ProcessManager* process_manager() { return ProcessManager::Get(profile()); }

  const extensions::Extension* extension_;
  std::unique_ptr<TestRenderViewContextMenu> menu_;
  std::unique_ptr<int> top_level_index_;
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
                       CreateOneVisibleTopLevelItem) {
  SetUpTestExtension();
  CallAPI("create({title: 'item', visible: true});");

  ASSERT_TRUE(SetupTopLevelMenuModel());

  VerifyNumContextMenuItems(1);

  VerifyMenuItem("item", top_level_model_, GetTopLevelIndex(),
                 ui::MenuModel::TYPE_COMMAND, true);

  // There should be no submenu model.
  ui::MenuModel* submodel =
      top_level_model_->GetSubmenuModelAt(GetTopLevelIndex());
  ASSERT_TRUE(submodel == NULL);
}

// Tests that a hidden menu item should not be added to the top-level menu
// model, which includes actions like "Back", "View Page Source", "Inspect",
// etc.
IN_PROC_BROWSER_TEST_F(ExtensionContextMenuApiTest,
                       DoNotCreateTopLevelItemIfHidden) {
  SetUpTestExtension();
  CallAPI("create({id: 'item1', title: 'item', visible: true});");
  CallAPI("update('item1', {visible: false});");

  ASSERT_TRUE(SetupTopLevelMenuModel());

  VerifyNumContextMenuItems(1);

  VerifyMenuItem("item", top_level_model_, GetTopLevelIndex(),
                 ui::MenuModel::TYPE_COMMAND, false);

  // There should be no submenu model.
  ui::MenuModel* submodel =
      top_level_model_->GetSubmenuModelAt(GetTopLevelIndex());
  ASSERT_TRUE(submodel == NULL);
}

// Tests that if a parent menu item and its children are both hidden, then the
// parent item should not be added to the top-level menu model, which includes
// actions like "Back", "View Page Source", "Inspect", etc. This also tests that
// no submenu model is created for the hidden children.
IN_PROC_BROWSER_TEST_F(
    ExtensionContextMenuApiTest,
    DoNotCreateTopLevelSubmenuItemIfHiddenAndChildrenHidden) {
  SetUpTestExtension();
  CallAPI("create({id: 'id', title: 'parent', visible: false});");
  CallAPI("create({title: 'child1', parentId: 'id', visible: false});");
  CallAPI("create({title: 'child2', parentId: 'id', visible: false});");

  ASSERT_TRUE(SetupTopLevelMenuModel());

  VerifyNumContextMenuItems(3);

  VerifyMenuItem("parent", top_level_model_, GetTopLevelIndex(),
                 ui::MenuModel::TYPE_SUBMENU, false);

  ui::MenuModel* submodel =
      top_level_model_->GetSubmenuModelAt(GetTopLevelIndex());
  ASSERT_TRUE(submodel);
  EXPECT_EQ(2, submodel->GetItemCount());

  VerifyMenuItem("child1", submodel, 0, ui::MenuModel::TYPE_COMMAND, false);
  VerifyMenuItem("child2", submodel, 1, ui::MenuModel::TYPE_COMMAND, false);
}

// Tests that if a parent menu item is hidden and some of its children are
// visible, then the parent item should not be added to the top-level menu
// model, which includes actions like "Back", "View Page Source", "Inspect",
// etc. This also tests that no submenu model is created for the children.
IN_PROC_BROWSER_TEST_F(
    ExtensionContextMenuApiTest,
    DoNotCreateTopLevelSubmenuItemIfHiddenAndSomeChildrenVisible) {
  SetUpTestExtension();
  CallAPI("create({id: 'id', title: 'parent', visible: false});");
  CallAPI("create({title: 'child1', parentId: 'id', visible: false});");
  CallAPI("create({title: 'child2', parentId: 'id', visible: true});");

  ASSERT_TRUE(SetupTopLevelMenuModel());

  VerifyNumContextMenuItems(3);

  VerifyMenuItem("parent", top_level_model_, GetTopLevelIndex(),
                 ui::MenuModel::TYPE_SUBMENU, false);

  ui::MenuModel* submodel =
      top_level_model_->GetSubmenuModelAt(GetTopLevelIndex());
  ASSERT_TRUE(submodel);
  EXPECT_EQ(2, submodel->GetItemCount());

  VerifyMenuItem("child1", submodel, 0, ui::MenuModel::TYPE_COMMAND, false);
  VerifyMenuItem("child2", submodel, 1, ui::MenuModel::TYPE_COMMAND, false);
}

// Tests that a single top-level parent menu item should be created when all of
// its child items are hidden. The child items' hidden states are tested too.
// Recall that a top-level item can be either a parent item specified by the
// developer or parent item labeled with the extension's name. In this case, we
// test the former.
IN_PROC_BROWSER_TEST_F(ExtensionContextMenuApiTest,
                       CreateTopLevelItemIfAllItsChildrenAreHidden) {
  SetUpTestExtension();
  CallAPI("create({id: 'id', title: 'parent', visible: true});");
  CallAPI("create({title: 'child1', parentId: 'id', visible: false});");
  CallAPI("create({title: 'child2', parentId: 'id', visible: false});");

  ASSERT_TRUE(SetupTopLevelMenuModel());

  VerifyNumContextMenuItems(3);

  VerifyMenuItem("parent", top_level_model_, GetTopLevelIndex(),
                 ui::MenuModel::TYPE_SUBMENU, true);

  ui::MenuModel* submodel =
      top_level_model_->GetSubmenuModelAt(GetTopLevelIndex());
  ASSERT_TRUE(submodel);
  EXPECT_EQ(2, submodel->GetItemCount());

  VerifyMenuItem("child1", submodel, 0, ui::MenuModel::TYPE_COMMAND, false);
  VerifyMenuItem("child2", submodel, 1, ui::MenuModel::TYPE_COMMAND, false);
}

// Tests that a top-level parent menu item should be created as a submenu,
// when some of its child items are visibile. The child items' visibilities are
// tested too. Recall that a top-level item can be either a parent item
// specified by the developer or parent item labeled with the extension's name.
// In this case, we test the former.
IN_PROC_BROWSER_TEST_F(ExtensionContextMenuApiTest,
                       CreateSubmenuTopLevelItemIfSomeOfChildrenAreVisible) {
  SetUpTestExtension();
  CallAPI("create({id: 'id', title: 'parent', visible: true});");
  CallAPI("create({title: 'child1', parentId: 'id', visible: true});");
  CallAPI("create({title: 'child2', parentId: 'id', visible: false});");

  ASSERT_TRUE(SetupTopLevelMenuModel());

  VerifyNumContextMenuItems(3);

  VerifyMenuItem("parent", top_level_model_, GetTopLevelIndex(),
                 ui::MenuModel::TYPE_SUBMENU, true);

  ui::MenuModel* submodel =
      top_level_model_->GetSubmenuModelAt(GetTopLevelIndex());
  ASSERT_TRUE(submodel);
  EXPECT_EQ(2, submodel->GetItemCount());

  VerifyMenuItem("child1", submodel, 0, ui::MenuModel::TYPE_COMMAND, true);
  VerifyMenuItem("child2", submodel, 1, ui::MenuModel::TYPE_COMMAND, false);
}

// Tests that no top-level parent menu item should be created when all of its
// child items are hidden. Recall that a top-level item can be either a parent
// item specified by the developer or parent item labeled with the extension's
// name. In this case, we test the latter.
IN_PROC_BROWSER_TEST_F(ExtensionContextMenuApiTest, CreateAllHiddenItems) {
  SetUpTestExtension();
  CallAPI("create({title: 'item1', visible: false});");
  CallAPI("create({title: 'item2', visible: false});");
  CallAPI("create({title: 'item3', visible: false});");

  ASSERT_TRUE(SetupTopLevelMenuModel());

  VerifyNumContextMenuItems(3);

  VerifyMenuItem(extension()->name(), top_level_model_, GetTopLevelIndex(),
                 ui::MenuModel::TYPE_SUBMENU, false);

  ui::MenuModel* submodel =
      top_level_model_->GetSubmenuModelAt(GetTopLevelIndex());
  ASSERT_TRUE(submodel);
  EXPECT_EQ(3, submodel->GetItemCount());

  VerifyMenuItem("item1", submodel, 0, ui::MenuModel::TYPE_COMMAND, false);
  VerifyMenuItem("item2", submodel, 1, ui::MenuModel::TYPE_COMMAND, false);
  VerifyMenuItem("item3", submodel, 2, ui::MenuModel::TYPE_COMMAND, false);
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
                       CreateWithCustomVisibility) {
  SetUpTestExtension();
  CallAPI("create({title: 'item1'});");
  CallAPI("create({title: 'item2'});");
  CallAPI("create({title: 'item3', id: 'item3', visible: false});");
  CallAPI("create({title: 'child1', visible: true, parentId: 'item3'});");
  CallAPI("create({title: 'child2', visible: true, parentId: 'item3'});");

  ASSERT_TRUE(SetupTopLevelMenuModel());

  VerifyNumContextMenuItems(5);

  VerifyMenuItem(extension()->name(), top_level_model_, GetTopLevelIndex(),
                 ui::MenuModel::TYPE_SUBMENU, true);

  ui::MenuModel* submodel =
      top_level_model_->GetSubmenuModelAt(GetTopLevelIndex());
  ASSERT_TRUE(submodel);
  EXPECT_EQ(3, submodel->GetItemCount());

  VerifyMenuItem("item1", submodel, 0, ui::MenuModel::TYPE_COMMAND, true);
  VerifyMenuItem("item2", submodel, 1, ui::MenuModel::TYPE_COMMAND, true);
  VerifyMenuItem("item3", submodel, 2, ui::MenuModel::TYPE_SUBMENU, false);

  ui::MenuModel* item3Submodel = submodel->GetSubmenuModelAt(2);
  ASSERT_TRUE(item3Submodel);
  EXPECT_EQ(2, item3Submodel->GetItemCount());

  VerifyMenuItem("child1", item3Submodel, 0, ui::MenuModel::TYPE_COMMAND,
                 false);
  VerifyMenuItem("child2", item3Submodel, 1, ui::MenuModel::TYPE_COMMAND,
                 false);
}

// Tests |visible| field in updateProperties (chrome.contextMenus.update).
IN_PROC_BROWSER_TEST_F(ExtensionContextMenuApiTest, UpdateItemVisibility) {
  SetUpTestExtension();
  CallAPI("create({title: 'item1'});");
  CallAPI("create({title: 'item2'});");
  CallAPI("create({title: 'item3', id: 'item3', visible: false});");
  CallAPI("create({title: 'child1', visible: true, parentId: 'item3'});");
  CallAPI("create({title: 'child2', visible: false, parentId: 'item3'});");
  CallAPI("update('item3', {visible: true});");

  ASSERT_TRUE(SetupTopLevelMenuModel());

  VerifyNumContextMenuItems(5);

  VerifyMenuItem(extension()->name(), top_level_model_, GetTopLevelIndex(),
                 ui::MenuModel::TYPE_SUBMENU, true);

  ui::MenuModel* submodel =
      top_level_model_->GetSubmenuModelAt(GetTopLevelIndex());
  ASSERT_TRUE(submodel);
  EXPECT_EQ(3, submodel->GetItemCount());

  VerifyMenuItem("item1", submodel, 0, ui::MenuModel::TYPE_COMMAND, true);
  VerifyMenuItem("item2", submodel, 1, ui::MenuModel::TYPE_COMMAND, true);
  VerifyMenuItem("item3", submodel, 2, ui::MenuModel::TYPE_SUBMENU, true);

  ui::MenuModel* item3Submodel = submodel->GetSubmenuModelAt(2);
  ASSERT_TRUE(item3Submodel);
  EXPECT_EQ(2, item3Submodel->GetItemCount());

  VerifyMenuItem("child1", item3Submodel, 0, ui::MenuModel::TYPE_COMMAND, true);
  VerifyMenuItem("child2", item3Submodel, 1, ui::MenuModel::TYPE_COMMAND,
                 false);
}

// Tests that hidden items are not counted against
// chrome.contextMenus.ACTION_MENU_TOP_LEVEL_LIMIT.
IN_PROC_BROWSER_TEST_F(ExtensionContextMenuApiTest, ItemLimitVisibility) {
  SetUpTestExtension();
  for (int i = 0; i < api::context_menus::ACTION_MENU_TOP_LEVEL_LIMIT; i++) {
    const std::string name("item" + std::to_string(i));
    CallAPI("create({title: '" + name + "', id: '" + name + "'});");
  }
  // Update and hide two items in the menu.
  CallAPI("update('item0', {visible: false});");
  CallAPI("update('item1', {visible: false});");

  // Create two items to replace of the two hidden items.
  CallAPI("create({title: 'newItem1'});");
  CallAPI("create({title: 'newItem2'});");

  ASSERT_TRUE(SetupTopLevelMenuModel());

  int expectedNumItems = api::context_menus::ACTION_MENU_TOP_LEVEL_LIMIT + 2;
  VerifyNumContextMenuItems(expectedNumItems);

  VerifyMenuItem(extension()->name(), top_level_model_, GetTopLevelIndex(),
                 ui::MenuModel::TYPE_SUBMENU, true);

  ui::MenuModel* submodel =
      top_level_model_->GetSubmenuModelAt(GetTopLevelIndex());
  ASSERT_TRUE(submodel);
  EXPECT_EQ(expectedNumItems, submodel->GetItemCount());

  VerifyMenuItem("item0", submodel, 0, ui::MenuModel::TYPE_COMMAND, false);
  VerifyMenuItem("item1", submodel, 1, ui::MenuModel::TYPE_COMMAND, false);

  VerifyMenuItem("item2", submodel, 2, ui::MenuModel::TYPE_COMMAND, true);
  VerifyMenuItem("item3", submodel, 3, ui::MenuModel::TYPE_COMMAND, true);
  VerifyMenuItem("item4", submodel, 4, ui::MenuModel::TYPE_COMMAND, true);
  VerifyMenuItem("item5", submodel, 5, ui::MenuModel::TYPE_COMMAND, true);

  VerifyMenuItem("newItem1", submodel, 6, ui::MenuModel::TYPE_COMMAND, true);
  VerifyMenuItem("newItem2", submodel, 7, ui::MenuModel::TYPE_COMMAND, true);
}

}  // namespace extensions

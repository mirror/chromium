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

int CountMenuItems(const ui::MenuModel* model) {
  int num_items_found = 0;
  if (model) {
    for (int i = 0; i < model->GetItemCount(); ++i) {
      ++num_items_found;
      num_items_found += CountMenuItems(model->GetSubmenuModelAt(i));
    }
  }
  return num_items_found;
}

int ConvertToExtensionsCustomCommandId(
    const std::shared_ptr<TestRenderViewContextMenu>& menu,
    int id) {
  return menu.get()
             ? menu->extension_items().ConvertToExtensionsCustomCommandId(id)
             : -1;
}

bool GetMenuModel(Browser* browser,
                  std::shared_ptr<TestRenderViewContextMenu>& menu,
                  ui::MenuModel** model) {
  content::RenderFrameHost* frame = GetWebContents(browser)->GetMainFrame();
  content::ContextMenuParams params;
  params.page_url = frame->GetLastCommittedURL();

  // Create context menu.
  menu.reset(new TestRenderViewContextMenu(frame, params));
  menu->Init();

  // Get context menu.
  int index = -1;
  return menu->GetMenuModelAndItemIndex(
      ConvertToExtensionsCustomCommandId(menu, 1), model, &index);
}

}  // namespace

namespace extensions {

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

// Tests |visible| field in createProperties (chrome.contextMenus.create). Also,
// tests that hiding a parent item should shide its children.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest,
                       ContextMenusCreateWithCustomVisibility) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(RunExtensionTest("context_menus/item_visibility/create_item"))
      << message_;
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Get context menu.
  std::shared_ptr<TestRenderViewContextMenu> menu;
  ui::MenuModel* model = nullptr;
  ASSERT_TRUE(GetMenuModel(browser(), menu, &model));
  ASSERT_TRUE(model);
  ASSERT_TRUE(menu.get());

  // There should be two menu items present. The third should be hidden.
  EXPECT_EQ(2, CountMenuItems(model));
  EXPECT_TRUE(
      menu->IsCommandIdVisible(ConvertToExtensionsCustomCommandId(menu, 1)));
  EXPECT_EQ(base::UTF8ToUTF16(std::string("item1")), model->GetLabelAt(0));
  EXPECT_TRUE(
      menu->IsCommandIdVisible(ConvertToExtensionsCustomCommandId(menu, 2)));
  EXPECT_EQ(base::UTF8ToUTF16(std::string("item2")), model->GetLabelAt(1));
}

// Tests |visible| field in updateProperties (chrome.contextMenus.update).
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, ContextMenusUpdateItemVisibility) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(RunExtensionTest("context_menus/item_visibility/update_item"))
      << message_;
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Get context menu.
  std::shared_ptr<TestRenderViewContextMenu> menu;
  ui::MenuModel* model = nullptr;
  ASSERT_TRUE(GetMenuModel(browser(), menu, &model));
  ASSERT_TRUE(model);
  ASSERT_TRUE(menu.get());

  // There should be four menu items present.
  EXPECT_EQ(4, CountMenuItems(model));

  // The first two items should be visible.
  EXPECT_TRUE(menu->IsCommandIdVisible(model->GetCommandIdAt(0)));
  EXPECT_EQ(base::UTF8ToUTF16(std::string("item1")), model->GetLabelAt(0));
  EXPECT_TRUE(menu->IsCommandIdVisible(model->GetCommandIdAt(1)));
  EXPECT_EQ(base::UTF8ToUTF16(std::string("item2")), model->GetLabelAt(1));

  // The third item is a parent with two child items.
  EXPECT_TRUE(menu->IsCommandIdVisible(model->GetCommandIdAt(2)));
  EXPECT_EQ(base::UTF8ToUTF16(std::string("item3")), model->GetLabelAt(2));
  // One child is visible, accounted as the fourth item in the  menu.
  EXPECT_TRUE(
      menu->IsCommandIdVisible(model->GetSubmenuModelAt(2)->GetCommandIdAt(0)));
  EXPECT_EQ(base::UTF8ToUTF16(std::string("child1")),
            model->GetSubmenuModelAt(2)->GetLabelAt(0));
  // The second child should be hidden. Note that because it is not included in
  // the menu model, we do not use the model->GetCommandIdAt accessor to convert
  // the command id to an extension custom id. Also note, there should be five
  // items in the menu (RenderViewContextMenu). Four are visible, and the fifth
  // is hidden; hence, the use of index 5 below.
  EXPECT_FALSE(
      menu->IsCommandIdVisible(ConvertToExtensionsCustomCommandId(menu, 5)));
}

// Tests that hidden items are not counted against
// chrome.contextMenus.ACTION_MENU_TOP_LEVEL_LIMIT.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, ContextMenusItemLimitVisibility) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(RunExtensionTest("context_menus/item_visibility/item_limit"))
      << message_;
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Get context menu.
  std::shared_ptr<TestRenderViewContextMenu> menu;
  ui::MenuModel* model = nullptr;
  ASSERT_TRUE(GetMenuModel(browser(), menu, &model));
  ASSERT_TRUE(model);
  ASSERT_TRUE(menu.get());

  // The model should show six items (ACTION_MENU_TOP_LEVEL_LIMIT = 6).
  EXPECT_EQ(api::context_menus::ACTION_MENU_TOP_LEVEL_LIMIT,
            CountMenuItems(model));

  // The first two items created should be hidden.
  //
  // Note that they will not be included in the menu model, so we do not use the
  // model->GetCommandIdAt accessor to convert the command id to an extension
  // custom id.
  //
  // Also note that because the visibility of the items was toggled via a
  // chrome.contextMenus.update(), the items will be at the end of the menu
  // list. There should be eight items in the menu (RenderViewContextMenu);
  // hence, the use of indicies 7 and 8 below.
  EXPECT_FALSE(
      menu->IsCommandIdVisible(ConvertToExtensionsCustomCommandId(menu, 7)));
  EXPECT_FALSE(
      menu->IsCommandIdVisible(ConvertToExtensionsCustomCommandId(menu, 8)));

  // The next four items created should be visisble.
  EXPECT_TRUE(menu->IsCommandIdVisible(model->GetCommandIdAt(0)));
  EXPECT_EQ(base::UTF8ToUTF16(std::string("item3")), model->GetLabelAt(0));
  EXPECT_TRUE(menu->IsCommandIdVisible(model->GetCommandIdAt(1)));
  EXPECT_EQ(base::UTF8ToUTF16(std::string("item4")), model->GetLabelAt(1));
  EXPECT_TRUE(menu->IsCommandIdVisible(model->GetCommandIdAt(2)));
  EXPECT_EQ(base::UTF8ToUTF16(std::string("item5")), model->GetLabelAt(2));
  EXPECT_TRUE(menu->IsCommandIdVisible(model->GetCommandIdAt(3)));
  EXPECT_EQ(base::UTF8ToUTF16(std::string("item6")), model->GetLabelAt(3));

  // The last two items that were added in place of the first two items should
  // be visible.
  EXPECT_TRUE(menu->IsCommandIdVisible(model->GetCommandIdAt(4)));
  EXPECT_EQ(base::UTF8ToUTF16(std::string("newItem1")), model->GetLabelAt(4));
  EXPECT_TRUE(menu->IsCommandIdVisible(model->GetCommandIdAt(5)));
  EXPECT_EQ(base::UTF8ToUTF16(std::string("newItem2")), model->GetLabelAt(5));
}

}  // namespace extensions

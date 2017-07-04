// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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

bool GetMenuModel(Browser* browser,
                  const extensions::Extension* extension,
                  ui::MenuModel** model) {
  // Setup page action.
  ui_test_utils::NavigateToURL(browser,
                               extension->GetResourceURL("popup.html"));
  content::RenderFrameHost* frame = GetWebContents(browser)->GetMainFrame();
  content::ContextMenuParams params;
  params.page_url = frame->GetLastCommittedURL();

  // Create context menu in page (popup.html).
  TestRenderViewContextMenu* menu =
      new TestRenderViewContextMenu(frame, params);
  menu->Init();

  // Get context menu.
  int index = -1;
  return menu->GetMenuModelAndItemIndex(
      menu->extension_items().ConvertToExtensionsCustomCommandId(1), model,
      &index);
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

  // Get context menu in page.
  ui::MenuModel* model = nullptr;
  ASSERT_TRUE(GetMenuModel(browser(), extension, &model));

  // There should be two menu items present. The third should be hidden.
  EXPECT_EQ(2, CountMenuItems(model));
  ResultCatcher catcher;
  ASSERT_TRUE(catcher.GetNextResult());
}

// Tests |visible| field in updateProperties (chrome.contextMenus.update).
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, ContextMenusUpdateItemVisibility) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(RunExtensionTest("context_menus/item_visibility/update_item"))
      << message_;
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Get context menu in page.
  ui::MenuModel* model = nullptr;
  ASSERT_TRUE(GetMenuModel(browser(), extension, &model));

  // There should be four menu items present. The third is a parent item with
  // two child items. One child is visible, accounted as the fourth item in the
  // menu. The second child should be hidden and not included in the count.
  EXPECT_EQ(4, CountMenuItems(model));
  ResultCatcher catcher;
  ASSERT_TRUE(catcher.GetNextResult());
}

// Tests that hidden items are not counted against
// chrome.contextMenus.ACTION_MENU_TOP_LEVEL_LIMIT.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, ContextMenusItemLimitVisibility) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(RunExtensionTest("context_menus/item_visibility/item_limit"))
      << message_;
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Get context menu in page.
  ui::MenuModel* model = nullptr;
  ASSERT_TRUE(GetMenuModel(browser(), extension, &model));

  EXPECT_EQ(api::context_menus::ACTION_MENU_TOP_LEVEL_LIMIT,
            CountMenuItems(model));
  ResultCatcher catcher;
  ASSERT_TRUE(catcher.GetNextResult());
}

}  // namespace extensions

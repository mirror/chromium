// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/fake_safe_browsing_database_manager.h"
#include "chrome/browser/extensions/test_extension_dir.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/blacklist_state.h"
#include "extensions/browser/extension_prefs.h"

namespace {

const char kUninstallUrl[] = "http://www.google.com/";

std::string GetActiveUrl(Browser* browser) {
  return browser->tab_strip_model()
      ->GetActiveWebContents()
      ->GetLastCommittedURL()
      .spec();
}

}  // namespace

// Tests that when a blacklisted extension with a set uninstall url is
// uninstalled, its uninstall url does not open.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest,
                       DoNotOpenUninstallUrlForBlacklistedExtensions) {
  // Load an extension that has set an uninstall url.
  scoped_refptr<const extensions::Extension> e =
      LoadExtension(test_data_dir_.AppendASCII("runtime")
                        .AppendASCII("uninstall_url")
                        .AppendASCII("sets_uninstall_url"));

  ASSERT_TRUE(e.get());
  extension_service()->AddExtension(e.get());
  ASSERT_TRUE(extension_service()->IsExtensionEnabled(e->id()));

  // Uninstall the extension and expect its uninstall url to open.
  extension_service()->UninstallExtension(
      e->id(), extensions::UNINSTALL_REASON_USER_INITIATED, NULL);
  TabStripModel* tabs = browser()->tab_strip_model();

  EXPECT_EQ(2, tabs->count());
  content::WaitForLoadStop(tabs->GetActiveWebContents());
  // Verify the uninstall url
  EXPECT_EQ(kUninstallUrl, GetActiveUrl(browser()));

  // Close the tab pointing to the uninstall url.
  tabs->CloseWebContentsAt(tabs->active_index(), 0);
  EXPECT_EQ(1, tabs->count());
  EXPECT_EQ("about:blank", GetActiveUrl(browser()));

  // Load the same extension again, except blacklist it after installation.
  e = LoadExtension(test_data_dir_.AppendASCII("runtime")
                        .AppendASCII("uninstall_url")
                        .AppendASCII("sets_uninstall_url"));
  extension_service()->AddExtension(e.get());
  ASSERT_TRUE(extension_service()->IsExtensionEnabled(e->id()));

  // Blacklist extension.
  extensions::ExtensionPrefs::Get(profile())->SetExtensionBlacklistState(
      e->id(), extensions::BlacklistState::BLACKLISTED_MALWARE);

  // Uninstalling a blacklisted extension should not open its uninstall url.
  extension_service()->UninstallExtension(
      e->id(), extensions::UNINSTALL_REASON_USER_INITIATED, NULL);
  EXPECT_EQ(1, tabs->count());
  content::WaitForLoadStop(tabs->GetActiveWebContents());
  EXPECT_EQ("about:blank", GetActiveUrl(browser()));
}

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <string>

#include "base/scoped_observer.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/extensions/browsertest_util.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "extensions/test/extension_test_message_listener.h"
#include "extensions/browser/state_store.h"

namespace extensions {

class TestObserver : public StateStore::Observer {
 public:
  TestObserver(content::BrowserContext* context,
               const std::string& extension_id) :
      extension_id_(extension_id),
      scoped_observer_(this) {
    scoped_observer_.Add(ExtensionSystem::Get(context)->state_store());
  }
  ~TestObserver() override {}

  void WillUpdateExtensionValue(const std::string& extension_id,
                                const std::string& key) override {
    if (extension_id == extension_id_)
      ++updated_values_[key];
  }

  int CountForKey(const std::string& key) const {
    auto iter = updated_values_.find(key);
    return iter == updated_values_.end() ? 0 : iter->second;
  }

 private:
  std::string extension_id_;
  std::map<std::string, int> updated_values_;

  ScopedObserver<StateStore, StateStore::Observer> scoped_observer_;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

using ExtensionActionAPITest = ExtensionApiTest;

IN_PROC_BROWSER_TEST_F(ExtensionActionAPITest, TestNoUnnecessaryIO) {
  ExtensionTestMessageListener ready_listener("ready", false);
  const Extension* extension =
      LoadExtension(
          test_data_dir_.AppendASCII("extension_action/unnecessary_io"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(ready_listener.WaitUntilSatisfied());

  constexpr char kUpdate[] =
      R"(console.warn('Running');
         chrome.browserAction.setBadgeText(%s);
         chrome.browserAction.getBadgeText(%s, function() {
           domAutomationController.send(
               chrome.runtime.lastError ?
                   chrome.runtime.lastError.message : 'pass');
         });)";
  TestObserver test_observer(profile(), extension->id());
  int tab_id = SessionTabHelper::IdForTab(
                   browser()->tab_strip_model()->GetActiveWebContents());
  std::string update_options =
      base::StringPrintf("{text: 'New Text', tabId: %d}", tab_id);
  std::string query_options =
      base::StringPrintf("{tabId: %d}", tab_id);
  LOG(WARNING) << "Injecting script";
  EXPECT_EQ(
      "pass",
      browsertest_util::ExecuteScriptInBackgroundPage(
          profile(), extension->id(),
          base::StringPrintf(kUpdate, update_options.c_str(), query_options.c_str())));

  constexpr char kBrowserActionKey[] = "browser_action";
  EXPECT_EQ(0, test_observer.CountForKey(kBrowserActionKey));

  EXPECT_EQ(
      "pass",
      browsertest_util::ExecuteScriptInBackgroundPage(
          profile(), extension->id(),
          base::StringPrintf(kUpdate, "{text: 'Default Text'}", "{}")));
  EXPECT_EQ(1, test_observer.CountForKey(kBrowserActionKey));
}

}  // namespace extensions

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/javascript_dialogs/javascript_dialog_tab_helper.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/policy_tool_ui_handler.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/base/l10n/l10n_util.h"

#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/test_extension_system.h"
#include "extensions/common/extension_builder.h"

namespace {
const std::string bookmarks_test_value =
    "[{\"toplevel_name\": \"My managed bookmarks folder\"}, "
    "{\"url\": \"google.com\", \"name\": \"Google\"}]";
}

class PolicyToolUITest : public InProcessBrowserTest {
 public:
  PolicyToolUITest();
  ~PolicyToolUITest() override;

 protected:
  // InProcessBrowserTest implementation.
  void SetUpInProcessBrowserTestFixture() override;
  void SetUp() override;

  base::FilePath GetSessionsDir();
  base::FilePath::StringType GetDefaultSessionName();
  base::FilePath::StringType GetSessionExtension();
  base::FilePath GetSessionPath(const base::FilePath::StringType& session_name);

  void LoadSession(const std::string& session_name);

  void LoadSessionAndWaitForAlert(const std::string& session_name);

  std::unique_ptr<base::DictionaryValue> ExtractPolicyValues(bool need_status);

  void SetPolicyValue(const std::string& policy_name,
                      const std::string& policy_value);

  void SetUpOnMainThread() override;

 protected:
  base::ScopedTempDir downloads_directory_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(PolicyToolUITest);
};

base::FilePath PolicyToolUITest::GetSessionsDir() {
  base::FilePath profile_dir;
  EXPECT_TRUE(PathService::Get(chrome::DIR_USER_DATA, &profile_dir));
  return profile_dir.AppendASCII(TestingProfile::kTestUserProfileDir)
      .Append(PolicyToolUIHandler::kPolicyToolSessionsDir);
}

base::FilePath::StringType PolicyToolUITest::GetDefaultSessionName() {
  return PolicyToolUIHandler::kPolicyToolDefaultSessionName;
}

base::FilePath::StringType PolicyToolUITest::GetSessionExtension() {
  return PolicyToolUIHandler::kPolicyToolSessionExtension;
}

base::FilePath PolicyToolUITest::GetSessionPath(
    const base::FilePath::StringType& session_name) {
  return GetSessionsDir()
      .Append(session_name)
      .AddExtension(GetSessionExtension());
}

PolicyToolUITest::PolicyToolUITest() {}

PolicyToolUITest::~PolicyToolUITest() {}

void PolicyToolUITest::SetUpInProcessBrowserTestFixture() {}

void PolicyToolUITest::SetUp() {
  scoped_feature_list_.InitAndEnableFeature(features::kPolicyTool);
  InProcessBrowserTest::SetUp();
}

void PolicyToolUITest::SetUpOnMainThread() {
  base::ScopedAllowBlockingForTesting allow_blocking;
  EXPECT_TRUE(downloads_directory_.CreateUniqueTempDir());
  browser()->profile()->GetPrefs()->SetFilePath(
      prefs::kDownloadDefaultDirectory, downloads_directory_.GetPath());
}

void PolicyToolUITest::LoadSession(const std::string& session_name) {
  const std::string javascript = "$('session-name-field').value = '" +
                                 session_name +
                                 "';"
                                 "$('load-session-button').click();";
  EXPECT_TRUE(content::ExecuteScript(
      browser()->tab_strip_model()->GetActiveWebContents(), javascript));
  content::RunAllTasksUntilIdle();
}

std::unique_ptr<base::DictionaryValue> PolicyToolUITest::ExtractPolicyValues(
    bool need_status) {
  std::string javascript =
      "var entries = document.querySelectorAll("
      "    'section.policy-table-section > * > tbody');"
      "var policies = {'chromePolicies': {}};"
      "for (var i = 0; i < entries.length; ++i) {"
      "  if (entries[i].hidden)"
      "    continue;"
      "  var name = entries[i].getElementsByClassName('name-column')[0]"
      "                       .getElementsByTagName('div')[0].textContent;"
      "  var valueContainer = entries[i].getElementsByClassName("
      "                                   'value-container')[0];"
      "  var value = valueContainer.getElementsByClassName("
      "                             'value')[0].textContent;";
  if (need_status) {
    javascript +=
        "  var status = entries[i].getElementsByClassName('status-column')[0]"
        "                         .getElementsByTagName('div')[0].textContent;"
        "  policies.chromePolicies[name] = {'value': value, 'status': status};";
  } else {
    javascript += "  policies.chromePolicies[name] = {'value': value};";
  }
  javascript += "} domAutomationController.send(JSON.stringify(policies));";
  std::string json;
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      browser()->tab_strip_model()->GetActiveWebContents(), javascript, &json));
  return base::DictionaryValue::From(base::JSONReader::Read(json));
}

void PolicyToolUITest::SetPolicyValue(const std::string& policy_name,
                                      const std::string& policy_value) {
  std::string javascript =
      "document.getElementById('show-unset').click();"
      "var policies = document.querySelectorAll("
      "    'section.policy-table-section > * > tbody');"
      "var policyEntry;"
      "for (var i = 0; i < policies.length; ++i) {"
      "  if (policies[i].getElementsByClassName('name')[0].textContent == '" +
      policy_name +
      "') {"
      "    policyEntry = policies[i];"
      "    break;"
      "  }"
      "}"
      "policyEntry.getElementsByClassName('edit-button')[0].click();"
      "policyEntry.getElementsByClassName('value-edit-field')[0].value = '" +
      policy_value +
      "';"
      "policyEntry.getElementsByClassName('save-button')[0].click();"
      "document.getElementById('show-unset').click();"
      "var name = policyEntry.getElementsByClassName('name-column')[0]"
      "                      .getElementsByTagName('div')[0].textContent;";
  EXPECT_TRUE(content::ExecuteScript(
      browser()->tab_strip_model()->GetActiveWebContents(), javascript));
  content::RunAllTasksUntilIdle();
}

void PolicyToolUITest::LoadSessionAndWaitForAlert(
    const std::string& session_name) {
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  JavaScriptDialogTabHelper* js_helper =
      JavaScriptDialogTabHelper::FromWebContents(contents);
  base::RunLoop dialog_wait;
  js_helper->SetDialogShownCallbackForTesting(dialog_wait.QuitClosure());
  LoadSession(session_name);
  dialog_wait.Run();
  EXPECT_TRUE(js_helper->IsShowingDialogForTesting());
}

IN_PROC_BROWSER_TEST_F(PolicyToolUITest, CreatingSessionFiles) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  // Check that the directory is not created yet.
  EXPECT_FALSE(PathExists(GetSessionsDir()));

  ui_test_utils::NavigateToURL(browser(), GURL("chrome://policy-tool"));

  // Check that the default session file was created.
  EXPECT_TRUE(PathExists(GetSessionPath(GetDefaultSessionName())));

  // Check that when moving to a different session the corresponding file is
  // created.
  LoadSession("test_creating_sessions");
  EXPECT_TRUE(
      PathExists(GetSessionPath(FILE_PATH_LITERAL("test_creating_sessions"))));

  // Check that unicode characters are valid for the session name.
  LoadSession("сессия");
  EXPECT_TRUE(PathExists(GetSessionPath(FILE_PATH_LITERAL("сессия"))));
}

IN_PROC_BROWSER_TEST_F(PolicyToolUITest, ImportingSession) {
  ui_test_utils::NavigateToURL(browser(), GURL("chrome://policy-tool"));

  // Set up policy values and put them in the session file.
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::DictionaryValue test_policies;
  // Add a known chrome policy.
  test_policies.SetString("chromePolicies.AllowDinosaurEasterEgg.value",
                          "true");
  // Add an unknown chrome policy.
  test_policies.SetString("chromePolicies.UnknownPolicy.value", "test");

  std::string json;
  base::JSONWriter::WriteWithOptions(
      test_policies, base::JSONWriter::Options::OPTIONS_PRETTY_PRINT, &json);
  base::WriteFile(GetSessionPath(FILE_PATH_LITERAL("test_session")),
                  json.c_str(), json.size());

  // Import the created session and wait until all the tasks are finished.
  LoadSession("test_session");

  std::unique_ptr<base::DictionaryValue> values = ExtractPolicyValues(false);
  EXPECT_EQ(test_policies, *values);
}

IN_PROC_BROWSER_TEST_F(PolicyToolUITest, Editing) {
  ui_test_utils::NavigateToURL(browser(), GURL("chrome://policy-tool"));

  // Change one policy values.
  SetPolicyValue("AllowDinosaurEasterEgg", "true");
  std::unique_ptr<base::Value> values = ExtractPolicyValues(true);
  base::DictionaryValue expected;
  expected.SetPath({"chromePolicies", "AllowDinosaurEasterEgg", "value"},
                   base::Value("true"));
  expected.SetPath({"chromePolicies", "AllowDinosaurEasterEgg", "status"},
                   base::Value(l10n_util::GetStringUTF8(IDS_POLICY_OK)));
  EXPECT_EQ(expected, *values);

  // Check if the session file is correct.
  base::ScopedAllowBlockingForTesting allow_blocking;
  std::string file_contents;
  base::ReadFileToString(GetSessionPath(GetDefaultSessionName()),
                         &file_contents);
  values = base::JSONReader::Read(file_contents);
  expected.SetDictionary("extensionPolicies",
                         base::MakeUnique<base::DictionaryValue>());
  expected.RemovePath({"chromePolicies", "AllowDinosaurEasterEgg", "status"});
  EXPECT_EQ(expected, *values);

  // Set value of this variable to "", so that it becomes unset.
  SetPolicyValue("AllowDinosaurEasterEgg", "");
  values = ExtractPolicyValues(false);
  expected.RemovePath({"chromePolicies", "AllowDinosaurEasterEgg"});
  expected.SetKey("chromePolicies", base::DictionaryValue());
  expected.RemoveKey("extensionPolicies");
  EXPECT_EQ(expected, *values);
}

IN_PROC_BROWSER_TEST_F(PolicyToolUITest, InvalidFilename) {
  ui_test_utils::NavigateToURL(browser(), GURL("chrome://policy-tool"));
  LoadSessionAndWaitForAlert("../test");
}

IN_PROC_BROWSER_TEST_F(PolicyToolUITest, InvalidJson) {
  ui_test_utils::NavigateToURL(browser(), GURL("chrome://policy-tool"));
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::WriteFile(GetSessionPath(FILE_PATH_LITERAL("test_session")), "{", 1);
  LoadSessionAndWaitForAlert("test_session");
}

IN_PROC_BROWSER_TEST_F(PolicyToolUITest, UnableToCreateDirectoryOrFile) {
  ui_test_utils::NavigateToURL(browser(), GURL("chrome://policy-tool"));
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::DeleteFile(GetSessionsDir(), true);
  base::File not_directory(GetSessionsDir(), base::File::Flags::FLAG_CREATE |
                                                 base::File::Flags::FLAG_WRITE);
  not_directory.Close();
  LoadSessionAndWaitForAlert("test_session");
}

IN_PROC_BROWSER_TEST_F(PolicyToolUITest, DefaultSession) {
  // Navigate to the tool to make sure the sessions directory is created.
  ui_test_utils::NavigateToURL(browser(), GURL("chrome://policy-tool"));

  // Check that if there are no sessions, a session with default name is
  // created.
  base::ScopedAllowBlockingForTesting allow_blocking;
  EXPECT_TRUE(base::PathExists(GetSessionPath(GetDefaultSessionName())));

  // Create a session file and load it.
  base::DictionaryValue expected;
  expected.SetString("chromePolicies.SessionName.value", "session");
  std::string file_contents;
  base::JSONWriter::Write(expected, &file_contents);

  base::WriteFile(GetSessionPath(FILE_PATH_LITERAL("session")),
                  file_contents.c_str(), file_contents.size());
  LoadSession("session");
  std::unique_ptr<base::DictionaryValue> values = ExtractPolicyValues(false);
  EXPECT_EQ(expected, *values);

  // Open the tool in a new tab and check that it loads the newly created
  // session (which is the last used session) and not the default session.
  chrome::NewTab(browser());
  ui_test_utils::NavigateToURL(browser(), GURL("chrome://policy-tool"));
  values = ExtractPolicyValues(false);
  EXPECT_EQ(expected, *values);
}

IN_PROC_BROWSER_TEST_F(PolicyToolUITest, MultipleSessionsChoice) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::CreateDirectory(GetSessionsDir());
  // Create 5 session files with different last access times and contents.
  base::DictionaryValue contents;
  base::Time initial_time = base::Time::Now();
  for (int i = 0; i < 5; ++i) {
    contents.SetPath({"chromePolicies", "SessionId", "value"},
                     base::Value(base::IntToString(i)));
    base::FilePath::StringType session_name =
        base::FilePath::FromUTF8Unsafe(base::IntToString(i)).value();
    std::string stringified_contents;
    base::JSONWriter::Write(contents, &stringified_contents);
    base::WriteFile(GetSessionPath(session_name), stringified_contents.c_str(),
                    stringified_contents.size());
    base::Time current_time = initial_time + base::TimeDelta::FromSeconds(i);
    base::TouchFile(GetSessionPath(session_name), current_time, current_time);
  }

  // Load the page. This should load the last session.
  ui_test_utils::NavigateToURL(browser(), GURL("chrome://policy-tool"));
  std::unique_ptr<base::DictionaryValue> page_contents =
      ExtractPolicyValues(false);
  EXPECT_EQ(contents, *page_contents);
}

IN_PROC_BROWSER_TEST_F(PolicyToolUITest, SessionTypeValidation) {
  // Load the page and check that the status is displayed correctly.
  ui_test_utils::NavigateToURL(browser(), GURL("chrome://policy-tool"));

  // Check that correct stringified boolean value is displayed correctly.
  SetPolicyValue("AllowDinosaurEasterEgg", "true");
  std::unique_ptr<base::DictionaryValue> page = ExtractPolicyValues(true);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_POLICY_OK)),
      *page->FindPath({"chromePolicies", "AllowDinosaurEasterEgg", "status"}));

  // Check that a non-boolean value is shown as invalid.
  SetPolicyValue("AllowDinosaurEasterEgg", "string");
  page = ExtractPolicyValues(true);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_POLICY_TOOL_INVALID_TYPE)),
      *page->FindPath({"chromePolicies", "AllowDinosaurEasterEgg", "status"}));

  // Check that a list value is parsed correctly.
  SetPolicyValue("ImagesAllowedForUrls", "[\"a\", \"b\"]");
  page = ExtractPolicyValues(true);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_POLICY_OK)),
      *page->FindPath({"chromePolicies", "ImagesAllowedForUrls", "status"}));

  // Check that a non-list value is considered invalid for list policy.
  SetPolicyValue("ImagesAllowedForUrls", "true");
  page = ExtractPolicyValues(true);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_POLICY_TOOL_INVALID_TYPE)),
      *page->FindPath({"chromePolicies", "ImagesAllowedForUrls", "status"}));

  // Check that a list of elements of wrong types is considered invalid.
  SetPolicyValue("ImagesAllowedForUrls", "[1, 2]");
  page = ExtractPolicyValues(true);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_POLICY_TOOL_INVALID_TYPE)),
      *page->FindPath({"chromePolicies", "ImagesAllowedForUrls", "status"}));

  // Check that a dictionary is parsed correctly.
  SetPolicyValue("ManagedBookmarks", bookmarks_test_value);
  page = ExtractPolicyValues(true);
  EXPECT_EQ(base::Value(l10n_util::GetStringUTF8(IDS_POLICY_OK)),
            *page->FindPath({"chromePolicies", "ManagedBookmarks", "status"}));

  // Check that a dictionary with wrong attributes is considered invalid.
  SetPolicyValue("ManagedBookmarks", "[{\"attribute\": \"value\"}]");
  page = ExtractPolicyValues(true);
  EXPECT_EQ(base::Value(l10n_util::GetStringUTF8(IDS_POLICY_TOOL_INVALID_TYPE)),
            *page->FindPath({"chromePolicies", "ManagedBookmarks", "status"}));
}

IN_PROC_BROWSER_TEST_F(PolicyToolUITest, ExportLinux) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir_;
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

  // Set up policy schema for extension.
  const std::string newly_added_policy_name = "new_policy";
  std::string json_data = "{\"type\": \"object\",\"properties\": {\"" +
                          newly_added_policy_name +
                          "\": { \"type\": \"string\"}}}";

  const std::string schema_file = "schema.json";
  base::FilePath schema_path = temp_dir_.GetPath().AppendASCII(schema_file);
  base::WriteFile(schema_path, json_data.data(), json_data.size());

  // Build extension.
  extensions::DictionaryBuilder storage;
  storage.Set("managed_schema", schema_file);

  extensions::DictionaryBuilder manifest;
  manifest.Set("name", "test")
      .Set("version", "1")
      .Set("manifest_version", 2)
      .Set("storage", storage.Build());

  extensions::ExtensionBuilder builder;
  builder.SetPath(temp_dir_.GetPath());
  builder.SetManifest(manifest.Build());
  
  // Generate a valid extension id.
  std::string extension_id = "";
  for (int i = 0; i < 32; ++i)
    extension_id += 'a';
  builder.SetID(extension_id);
  
  // Install extension.
  ExtensionService* service =
      extensions::ExtensionSystem::Get(browser()->profile())
          ->extension_service();
  service->OnExtensionInstalled(builder.Build().get(), syncer::StringOrdinal(),
                                0);

  ui_test_utils::NavigateToURL(browser(), GURL("chrome://policy-tool"));

  // Set policies of different data types.
  SetPolicyValue("AllowDinosaurEasterEgg", "true");
  SetPolicyValue("ImagesAllowedForUrls", "[\"a\", \"b\"]");

  SetPolicyValue("ManagedBookmarks", bookmarks_test_value);
  SetPolicyValue("new_policy", "string");

  // Export policies in Linux format.
  EXPECT_TRUE(
      ExecuteScript(browser()->tab_strip_model()->GetActiveWebContents(),
                    "$('export-linux').click();"));
  // Wait for the backend to process policy values.
  content::RunAllTasksUntilIdle();
  // Wait for the policy.Page.downloadFile call to complete.
  EXPECT_TRUE(
      ExecuteScript(browser()->tab_strip_model()->GetActiveWebContents(), ""));
  // Wait for the download to finish.
  content::RunAllTasksUntilIdle();

  std::string contents;
  // TODO(urusant): create a constant for default filename, which is common for
  // JS and test.
  base::ReadFileToString(
      downloads_directory_.GetPath().Append(FILE_PATH_LITERAL("policies.json")),
      &contents);
  std::unique_ptr<base::Value> linux_dict = base::JSONReader::Read(contents);
  EXPECT_TRUE(linux_dict);

  // Set up expected dictionary.
  base::DictionaryValue expected;
  expected.SetKey("AllowDinosaurEasterEgg", base::Value(true));
  base::ListValue list_of_strings;
  list_of_strings.GetList().push_back(base::Value("a"));
  list_of_strings.GetList().push_back(base::Value("b"));
  expected.SetKey("ImagesAllowedForUrls", std::move(list_of_strings));
  std::unique_ptr<base::Value> bookmarks =
      base::JSONReader::Read(bookmarks_test_value);
  expected.SetKey("ManagedBookmarks", std::move(*bookmarks));
  expected.SetPath({"3rdparty", "extensions", extension_id, "new_policy"},
                   base::Value("string"));

  EXPECT_EQ(expected, *linux_dict);
}
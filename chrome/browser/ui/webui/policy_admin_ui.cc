// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "chrome/browser/ui/webui/policy_admin_ui.h"

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/policy_ui_handler.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui.h"

namespace {

content::WebUIDataSource* CreatePolicyAdminUIHtmlSource() {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIPolicyAdminHost);
  PolicyUIHandler::AddCommonLocalizedStringsToSource(source);

  source->AddLocalizedString("reloadPolicies", IDS_POLICY_RELOAD_POLICIES);
  source->AddLocalizedString("showUnset", IDS_POLICY_SHOW_UNSET);
  source->AddLocalizedString("noPoliciesSet", IDS_POLICY_NO_POLICIES_SET);
  source->AddLocalizedString("showExpandedValue",
                             IDS_POLICY_SHOW_EXPANDED_VALUE);
  source->AddLocalizedString("hideExpandedValue",
                             IDS_POLICY_HIDE_EXPANDED_VALUE);
  source->AddLocalizedString("loadSession", IDS_POLICY_ADMIN_LOAD_SESSION);
  source->AddLocalizedString("sessionNamePlaceholder",
                             IDS_POLICY_ADMIN_SESSION_NAME_PLACEHOLDER);
  source->AddLocalizedString("filterPlaceholder",
                             IDS_POLICY_FILTER_PLACEHOLDER);
  source->AddLocalizedString("edit", IDS_POLICY_ADMIN_EDIT);
  source->AddLocalizedString("save", IDS_POLICY_ADMIN_SAVE);
  // Overwrite the title value added by PolicyUIHandler.
  source->AddLocalizedString("title", IDS_POLICY_ADMIN_TITLE);

  // Add required resources.
  source->AddResourcePath("policy_admin.css", IDR_POLICY_ADMIN_CSS);
  source->AddResourcePath("policy_admin.js", IDR_POLICY_ADMIN_JS);
  source->SetDefaultResource(IDR_POLICY_ADMIN_HTML);
  return source;
}

}  // namespace

class PolicyAdminUIHandler : public PolicyUIHandler {
 public:
  PolicyAdminUIHandler();
  ~PolicyAdminUIHandler() override;

  // content::WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void ImportFile();
  void HandleInitializedAdmin(const base::ListValue* args);
  void HandleLoadSession(const base::ListValue* args);
  void HandleUpdateSession(const base::ListValue* args);

  std::string ReadFromFileCallback();
  void OnFileRead(const std::string& contents);

  void DoUpdateSession(const std::string& contents);

  base::FilePath sessions_dir_;
  std::string session_name_;
};

PolicyAdminUIHandler::PolicyAdminUIHandler() {}

PolicyAdminUIHandler::~PolicyAdminUIHandler() {}

void PolicyAdminUIHandler::RegisterMessages() {
  // Set directory for storing sessions.
  sessions_dir_ =
      Profile::FromWebUI(web_ui())->GetPath().AppendASCII("Policy sessions");
  // Set current session name.
  // TODO: do so in a smarter way, e.g. choose the last edited session.
  session_name_ = "policy";

  web_ui()->RegisterMessageCallback(
      "initializedAdmin",
      base::Bind(&PolicyAdminUIHandler::HandleInitializedAdmin,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "loadSession", base::Bind(&PolicyAdminUIHandler::HandleLoadSession,
                                base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "updateSession", base::Bind(&PolicyAdminUIHandler::HandleUpdateSession,
                                  base::Unretained(this)));
}

std::string PolicyAdminUIHandler::ReadFromFileCallback() {
  // Create sessions directory, if it doesn't exist yet.
  if (!base::DirectoryExists(sessions_dir_))
    base::CreateDirectory(sessions_dir_);

  const base::FilePath session_path =
      sessions_dir_.AppendASCII(session_name_ + ".json");

  // Check if the file for the current session already exists. If not, create it
  //  and put an empty dictionary in it.
  base::File session(session_path, base::File::Flags::FLAG_CREATE |
                                       base::File::Flags::FLAG_WRITE);
  if (session.created()) {
    session.WriteAtCurrentPos("{}", 2);
    return "{}";
  }
  session.Close();

  // Read file contents.
  std::string contents;
  base::ReadFileToString(session_path, &contents);
  return contents;
}

void PolicyAdminUIHandler::OnFileRead(const std::string& contents) {
  std::unique_ptr<base::Value> value = base::JSONReader::Read(contents);
  AllowJavascript();
  CallJavascriptFunction("policy.Page.setPolicyValues", *value);
}

void PolicyAdminUIHandler::ImportFile() {
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&PolicyAdminUIHandler::ReadFromFileCallback,
                     base::Unretained(this)),
      base::BindOnce(&PolicyAdminUIHandler::OnFileRead,
                     base::Unretained(this)));
}

void PolicyAdminUIHandler::HandleInitializedAdmin(const base::ListValue* args) {
  SendPolicyNames();
  ImportFile();
}

void PolicyAdminUIHandler::HandleLoadSession(const base::ListValue* args) {
  args->GetString(0, &session_name_);
  ImportFile();
}

void PolicyAdminUIHandler::DoUpdateSession(const std::string& contents) {
  base::WriteFile(sessions_dir_.AppendASCII(session_name_ + ".json"),
                  contents.c_str(), contents.size());
}

void PolicyAdminUIHandler::HandleUpdateSession(const base::ListValue* args) {
  const base::DictionaryValue* policy_values;
  args->GetDictionary(0, &policy_values);
  std::string converted_values;
  base::JSONWriter::WriteWithOptions(
      *policy_values, base::JSONWriter::Options::OPTIONS_PRETTY_PRINT,
      &converted_values);
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&PolicyAdminUIHandler::DoUpdateSession,
                     base::Unretained(this), converted_values));
}

PolicyAdminUI::PolicyAdminUI(content::WebUI* web_ui) : WebUIController(web_ui) {
  web_ui->AddMessageHandler(base::MakeUnique<PolicyAdminUIHandler>());
  // web_ui->AddMessageHandler(base::MakeUnique<PolicyUIHandler>());
  content::WebUIDataSource::Add(Profile::FromWebUI(web_ui),
                                CreatePolicyAdminUIHtmlSource());
}

PolicyAdminUI::~PolicyAdminUI() {}

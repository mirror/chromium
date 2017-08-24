// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "chrome/browser/ui/webui/policy_tool_ui_handler.h"

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/profiles/profile.h"

PolicyToolUIHandler::PolicyToolUIHandler() {}

PolicyToolUIHandler::~PolicyToolUIHandler() {}

void PolicyToolUIHandler::RegisterMessages() {
  // Set directory for storing sessions.
  sessions_dir_ =
      Profile::FromWebUI(web_ui())->GetPath().Append("Policy sessions");
  // Set current session name.
  // TODO(urusant): do so in a smarter way, e.g. choose the last edited session.
  session_name_ = "policy";

  web_ui()->RegisterMessageCallback(
      "initialized", base::Bind(&PolicyToolUIHandler::HandleInitializedAdmin,
                                base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "loadSession", base::Bind(&PolicyToolUIHandler::HandleLoadSession,
                                base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "updateSession", base::Bind(&PolicyToolUIHandler::HandleUpdateSession,
                                  base::Unretained(this)));
}

std::string PolicyToolUIHandler::ReadOrCreateFileCallback() {
  // Create sessions directory, if it doesn't exist yet.
  // If unable to create a directory, just silently return an empty dictionary.
  // TODO(urusant): add a possibility to disable logging to disk in similar
  // cases.
  if (!base::CreateDirectory(sessions_dir_))
    return "{\"logged\": false}";

  const base::FilePath session_path =
      sessions_dir_.Append(session_name_ + ".json");
  // Check if the file for the current session already exists. If not, create it
  // and put an empty dictionary in it.
  base::File session_file(session_path, base::File::Flags::FLAG_CREATE |
                                            base::File::Flags::FLAG_WRITE);

  // If unable to open the file, just return an empty dictionary.
  if (session_file.created()) {
    session_file.WriteAtCurrentPos("{}", 2);
    return "{}";
  }
  session_file.Close();

  // Check that the file exists by now. If it doesn't, it means that at least
  // one of the filesystem operations wasn't successful. In this case, return
  // an empty dictionary. Potentially this can also be the place to disable
  // logging to disk.
  if (!PathExists(session_path)) {
    return "{\"logged\": false}";
  }
  // Read file contents.
  std::string contents;
  base::ReadFileToString(session_path, &contents);
  return contents;
}

void PolicyToolUIHandler::OnFileRead(const std::string& contents) {
  AllowJavascript();
  std::unique_ptr<base::DictionaryValue> value =
      base::DictionaryValue::From(base::JSONReader::Read(contents));

  // If contents is not a properly formed JSON string, we alert the user about
  // it and send an empty dictionary instead. Note that the broken session file
  // would be overrided when the user edits something, so any manual changes
  // that made it invalid will be lost.
  // TODO(urusant): do it in a smarter way, e.g. revert to a previous session or
  // a new session with a generated name.
  if (!value) {
    value = base::MakeUnique<base::DictionaryValue>();
    ShowErrorMessageToUser(
        "The file seems to be broken. Please note that any "
        "changes you make will override it.");
  } else {
    bool logged;
    if (value->GetBoolean("logged", &logged)) {
      if (!logged) {
        ShowErrorMessageToUser(
            "There seems to be a problem. Logging the "
            "session to disk might not work properly.");
      }
      value->Remove("logged", nullptr);
    }
  }
  CallJavascriptFunction("policy.Page.setPolicyValues", *value);
}

void PolicyToolUIHandler::ImportFile() {
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&PolicyToolUIHandler::ReadOrCreateFileCallback,
                     base::Unretained(this)),
      base::BindOnce(&PolicyToolUIHandler::OnFileRead, base::Unretained(this)));
}

void PolicyToolUIHandler::HandleInitializedAdmin(const base::ListValue* args) {
  DCHECK_EQ(0U, args->GetSize());
  SendPolicyNames();
  ImportFile();
}

bool PolicyToolUIHandler::IsValidSessionName(const std::string& name) const {
  base::FilePath session_path = sessions_dir_.Append(name);
  return session_path.DirName() == sessions_dir_;
}

void PolicyToolUIHandler::HandleLoadSession(const base::ListValue* args) {
  DCHECK_EQ(1U, args->GetSize());
  std::string new_session_name = args->GetList()[0].GetString();
  if (!IsValidSessionName(new_session_name)) {
    ShowErrorMessageToUser(
        "The session name is invalid. Please enter a valid session name.");
    return;
  }
  session_name_ = new_session_name;
  ImportFile();
}

void PolicyToolUIHandler::DoUpdateSession(const std::string& contents) {
  base::WriteFile(sessions_dir_.Append(session_name_ + ".json"),
                  contents.c_str(), contents.size());
}

void PolicyToolUIHandler::HandleUpdateSession(const base::ListValue* args) {
  DCHECK_EQ(1U, args->GetSize());

  const base::DictionaryValue* policy_values = nullptr;
  DCHECK(args->GetDictionary(0, &policy_values));
  std::string converted_values;
  base::JSONWriter::WriteWithOptions(
      *policy_values, base::JSONWriter::Options::OPTIONS_PRETTY_PRINT,
      &converted_values);
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&PolicyToolUIHandler::DoUpdateSession,
                     base::Unretained(this), converted_values));
}

void PolicyToolUIHandler::ShowErrorMessageToUser(const std::string& message) {
  AllowJavascript();
  CallJavascriptFunction("alert", base::Value(message));
}

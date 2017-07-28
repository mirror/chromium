// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/web_ui_message_handler.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"

namespace content {

void WebUIMessageHandler::AllowJavascriptForTesting() {
  AllowJavascript();
}

void WebUIMessageHandler::AllowJavascript() {
  if (javascript_allowed_)
    return;

  javascript_allowed_ = true;
  CHECK(IsJavascriptAllowed());

  OnJavascriptAllowed();
}

void WebUIMessageHandler::DisallowJavascript() {
  if (!javascript_allowed_)
    return;

  javascript_allowed_ = false;
  DCHECK(!IsJavascriptAllowed());

  for (size_t i = 0; i < callbacks_.size(); ++i)
    callbacks_[i].clear();

  OnJavascriptDisallowed();
}

bool WebUIMessageHandler::IsJavascriptAllowed() const {
  return javascript_allowed_ && web_ui() && web_ui()->CanCallJavascript();
}

const WebUICallback* WebUIMessageHandler::GetCallbackFromArgs(
    const base::ListValue* args) {
  double callback_id;
  if (!args || args->empty() || !args->GetDouble(0, &callback_id)) {
    NOTREACHED();
    callbacks_.emplace_back();
  } else {
    callbacks_.emplace_back(static_cast<int>(callback_id));
  }
  return &callbacks_.back();
}

bool WebUIMessageHandler::ExtractIntegerValue(const base::ListValue* value,
                                              int* out_int) {
  std::string string_value;
  if (value->GetString(0, &string_value))
    return base::StringToInt(string_value, out_int);
  double double_value;
  if (value->GetDouble(0, &double_value)) {
    *out_int = static_cast<int>(double_value);
    return true;
  }
  NOTREACHED();
  return false;
}

bool WebUIMessageHandler::ExtractDoubleValue(const base::ListValue* value,
                                             double* out_value) {
  std::string string_value;
  if (value->GetString(0, &string_value))
    return base::StringToDouble(string_value, out_value);
  if (value->GetDouble(0, out_value))
    return true;
  NOTREACHED();
  return false;
}

base::string16 WebUIMessageHandler::ExtractStringValue(
    const base::ListValue* value) {
  base::string16 string16_value;
  if (value->GetString(0, &string16_value))
    return string16_value;
  NOTREACHED();
  return base::string16();
}

void WebUIMessageHandler::ResolveJavascriptCallback(
    WebUICallback* callback,
    const base::Value& response) {
  if (!callback || callback->empty())
    return;
  // cr.webUIResponse is a global JS function exposed from cr.js.
  CallJavascriptFunction("cr.webUIResponse", base::StringValue(*callback),
                         base::Value(true), response);
  callback->clear();
}

void WebUIMessageHandler::RejectJavascriptCallback(
    WebUICallback* callback,
    const base::Value& response) {
  if (!callback || callback->empty())
    return;
  // cr.webUIResponse is a global JS function exposed from cr.js.
  CallJavascriptFunction("cr.webUIResponse", base::StringValue(*callback),
                         base::Value(false), response);
  callback->clear();
}

}  // namespace content

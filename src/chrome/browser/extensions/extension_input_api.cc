// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_input_api.h"

#include <string>

#include "base/string_util.h"
#include "base/values.h"
#include "chrome/browser/extensions/key_identifier_conversion_views.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/chrome_notification_types.h"
#include "views/events/event.h"
#include "views/ime/input_method.h"
#include "views/widget/widget.h"

#if defined(TOUCH_UI)
#include "content/common/notification_service.h"
#endif

#if defined(OS_CHROMEOS) && defined(TOUCH_UI)
#include "chrome/browser/chromeos/input_method/input_method_manager.h"
#include "chrome/browser/chromeos/input_method/ibus_controller.h"
#include "chrome/browser/chromeos/login/webui_login_display.h"
#endif

namespace {

// Keys.
const char kType[] = "type";
const char kKeyIdentifier[] = "keyIdentifier";
const char kAlt[] = "altKey";
const char kCtrl[] = "ctrlKey";
const char kMeta[] = "metaKey";
const char kShift[] = "shiftKey";
const char kKeyDown[] = "keydown";
const char kKeyUp[] = "keyup";

// Errors.
const char kUnknownEventTypeError[] = "Unknown event type.";
const char kUnknownOrUnsupportedKeyIdentiferError[] = "Unknown or unsupported "
    "key identifier.";
const char kUnsupportedModifier[] = "Unsupported modifier.";
const char kNoValidRecipientError[] = "No valid recipient for event.";
const char kKeyEventUnprocessedError[] = "Event was not handled.";
const char kInvalidHeight[] = "Invalid height.";

ui::EventType GetTypeFromString(const std::string& type) {
  if (type == kKeyDown) {
    return ui::ET_KEY_PRESSED;
  } else if (type == kKeyUp) {
    return ui::ET_KEY_RELEASED;
  }
  return ui::ET_UNKNOWN;
}

}  // namespace

void InputFunction::Run() {
  SendResponse(RunImpl());
}

views::Widget* SendKeyboardEventInputFunction::GetTopLevelWidget() {
#if defined(OS_CHROMEOS) && defined(TOUCH_UI)
  views::Widget* login_window = chromeos::WebUILoginDisplay::GetLoginWindow();
  if (login_window)
    return login_window;
#endif

  Browser* browser = GetCurrentBrowser();
  if (!browser)
    return NULL;

  BrowserWindow* window = browser->window();
  if (!window)
    return NULL;

  BrowserView* browser_view = BrowserView::GetBrowserViewForNativeWindow(
      window->GetNativeHandle());
  return browser_view ? browser_view->GetWidget() : NULL;
}

bool SendKeyboardEventInputFunction::RunImpl() {
  DictionaryValue* args;
  EXTENSION_FUNCTION_VALIDATE(args_->GetDictionary(0, &args));

  std::string type_name;
  EXTENSION_FUNCTION_VALIDATE(args->GetString(kType, &type_name));
  ui::EventType type = GetTypeFromString(type_name);
  if (type == ui::ET_UNKNOWN) {
    error_ = kUnknownEventTypeError;
    return false;
  }

  std::string identifier;
  EXTENSION_FUNCTION_VALIDATE(args->GetString(kKeyIdentifier, &identifier));
  TrimWhitespaceASCII(identifier, TRIM_ALL, &identifier);

  const views::KeyEvent& prototype_event =
      KeyEventFromKeyIdentifier(identifier);
  if (prototype_event.key_code() == ui::VKEY_UNKNOWN) {
    error_ = kUnknownOrUnsupportedKeyIdentiferError;
    return false;
  }

  bool flag = false;
  int flags = prototype_event.flags();
  flags |= (args->GetBoolean(kAlt, &flag) && flag) ? ui::EF_ALT_DOWN : 0;
  flags |= (args->GetBoolean(kCtrl, &flag) && flag) ? ui::EF_CONTROL_DOWN : 0;
  flags |= (args->GetBoolean(kShift, &flag) && flag) ? ui::EF_SHIFT_DOWN : 0;
  if (args->GetBoolean(kMeta, &flag) && flag) {
    // Views does not have a Meta event flag, so return an error for now.
    error_ = kUnsupportedModifier;
    return false;
  }

  views::Widget* widget = GetTopLevelWidget();
  if (!widget) {
    error_ = kNoValidRecipientError;
    return false;
  }

  views::KeyEvent event(type, prototype_event.key_code(), flags);
  views::InputMethod* ime = widget->GetInputMethod();
  if (ime) {
    ime->DispatchKeyEvent(event);
  } else if (!widget->OnKeyEvent(event)) {
    error_ = kKeyEventUnprocessedError;
    return false;
  }

  return true;
}

#if defined(TOUCH_UI)
bool HideKeyboardFunction::RunImpl() {
  NotificationService::current()->Notify(
      chrome::NOTIFICATION_HIDE_KEYBOARD_INVOKED,
      Source<HideKeyboardFunction>(this),
      NotificationService::NoDetails());
  return true;
}

bool SetKeyboardHeightFunction::RunImpl() {
  int height = 0;
  EXTENSION_FUNCTION_VALIDATE(args_->GetInteger(0, &height));

  if (height < 0) {
    error_ = kInvalidHeight;
    return false;
  }

  // TODO(penghuang) Check the height is not greater than height of browser view
  // and set the height of virtual keyboard directly instead of using
  // notification.
  NotificationService::current()->Notify(
      chrome::NOTIFICATION_SET_KEYBOARD_HEIGHT_INVOKED,
      Source<SetKeyboardHeightFunction>(this),
      Details<int>(&height));
  return true;
}
#endif

#if defined(OS_CHROMEOS) && defined(TOUCH_UI)
// TODO(yusukes): This part should be moved to extension_input_api_chromeos.cc.
bool SendHandwritingStrokeFunction::RunImpl() {
  // TODO(yusukes): Add a parameter for an input context ID.
  ListValue* value = NULL;
  EXTENSION_FUNCTION_VALIDATE(args_->GetList(0, &value));

  chromeos::input_method::HandwritingStroke stroke;
  for (size_t i = 0; i < value->GetSize(); ++i) {
    DictionaryValue* dict;
    double x = 0.0;
    double y = 0.0;
    EXTENSION_FUNCTION_VALIDATE(value->GetDictionary(i, &dict));
    EXTENSION_FUNCTION_VALIDATE(dict->GetDouble("x", &x));
    EXTENSION_FUNCTION_VALIDATE(dict->GetDouble("y", &y));
    stroke.push_back(std::make_pair(x, y));
  }
  chromeos::input_method::InputMethodManager::GetInstance()->
      SendHandwritingStroke(stroke);
  return true;
}

bool CancelHandwritingStrokesFunction::RunImpl() {
  // TODO(yusukes): Add a parameter for an input context ID.
  int stroke_count = 0;  // zero means 'clear all strokes'.
  if (HasOptionalArgument(0)) {
    EXTENSION_FUNCTION_VALIDATE(args_->GetInteger(0, &stroke_count));
    EXTENSION_FUNCTION_VALIDATE(stroke_count >= 0);
  }
  chromeos::input_method::InputMethodManager::GetInstance()->
      CancelHandwritingStrokes(stroke_count);
  return true;
}
#endif

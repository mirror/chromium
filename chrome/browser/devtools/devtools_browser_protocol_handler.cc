// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/devtools_browser_protocol_handler.h"

#include "chrome/browser/devtools/devtools_protocol.h"
#include "chrome/browser/devtools/devtools_protocol_constants.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_context.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/devtools_agent_host.h"

namespace {

BrowserWindow* GetBrowserWindow(int window_id) {
  for (auto* b : *BrowserList::GetInstance()) {
    if (b->session_id().id() == window_id)
      return b->window();
  }
  return nullptr;
}

// Get the bounds and state of the browser window. The bounds is for the
// restored window when the window is minimized. Otherwise, it is for the actual
// window.
std::unique_ptr<base::DictionaryValue> GetBounds(BrowserWindow* window) {
  gfx::Rect bounds;
  if (window->IsMinimized())
    bounds = window->GetRestoredBounds();
  else
    bounds = window->GetBounds();

  auto bounds_object = base::MakeUnique<base::DictionaryValue>();

  bounds_object->SetInteger("left", bounds.x());
  bounds_object->SetInteger("top", bounds.y());
  bounds_object->SetInteger("width", bounds.width());
  bounds_object->SetInteger("height", bounds.height());

  std::string window_state = "normal";
  if (window->IsMinimized())
    window_state = "minimized";
  if (window->IsMaximized())
    window_state = "maximized";
  if (window->IsFullscreen())
    window_state = "fullscreen";
  bounds_object->SetString("windowState", window_state);

  return bounds_object;
}

std::unique_ptr<base::DictionaryValue> GetWindowForTarget(
    int id,
    const base::DictionaryValue& params) {
  std::string target_id;
  if (!params.GetString("targetId", &target_id))
    return DevToolsProtocol::CreateInvalidParamsResponse(id, "targetId");

  Browser* browser = nullptr;
  scoped_refptr<content::DevToolsAgentHost> host =
      content::DevToolsAgentHost::GetForId(target_id);
  if (!host)
    return DevToolsProtocol::CreateErrorResponse(id, "No target with given id");
  content::WebContents* web_contents = host->GetWebContents();
  if (!web_contents) {
    return DevToolsProtocol::CreateErrorResponse(
        id, "No web contents in the target");
  }
  for (auto* b : *BrowserList::GetInstance()) {
    int tab_index = b->tab_strip_model()->GetIndexOfWebContents(web_contents);
    if (tab_index != TabStripModel::kNoTab)
      browser = b;
  }
  if (!browser) {
    return DevToolsProtocol::CreateErrorResponse(id,
                                                 "Browser window not found");
  }

  auto result = base::MakeUnique<base::DictionaryValue>();
  result->SetInteger("windowId", browser->session_id().id());
  result->Set("bounds", GetBounds(browser->window()));
  return DevToolsProtocol::CreateSuccessResponse(id, std::move(result));
}

std::unique_ptr<base::DictionaryValue> GetWindowBounds(
    int id,
    const base::DictionaryValue& params) {
  int window_id;
  if (!params.GetInteger("windowId", &window_id))
    return DevToolsProtocol::CreateInvalidParamsResponse(id, "windowId");
  BrowserWindow* window = GetBrowserWindow(window_id);
  if (!window) {
    return DevToolsProtocol::CreateErrorResponse(id,
                                                 "Browser window not found");
  }

  auto result = base::MakeUnique<base::DictionaryValue>();
  result->Set("bounds", GetBounds(window));
  return DevToolsProtocol::CreateSuccessResponse(id, std::move(result));
}

std::unique_ptr<base::DictionaryValue> SetWindowBounds(
    int id,
    const base::DictionaryValue& params) {
  int window_id;
  if (!params.GetInteger("windowId", &window_id))
    return DevToolsProtocol::CreateInvalidParamsResponse(id, "windowId");
  BrowserWindow* window = GetBrowserWindow(window_id);
  if (!window) {
    return DevToolsProtocol::CreateErrorResponse(id,
                                                 "Browser window not found");
  }

  const base::Value* value = nullptr;
  const base::DictionaryValue* bounds_dict = nullptr;
  if (!params.Get("bounds", &value) || !value->GetAsDictionary(&bounds_dict))
    return DevToolsProtocol::CreateInvalidParamsResponse(id, "bounds");

  std::string window_state;
  if (!bounds_dict->GetString("windowState", &window_state))
    window_state = "normal";
  else if (window_state != "normal" && window_state != "minimized" &&
           window_state != "maximized" && window_state != "fullscreen")
    return DevToolsProtocol::CreateInvalidParamsResponse(id, "windowState");

  // Compute updated bounds when window state is normal.
  bool set_bounds = false;
  gfx::Rect bounds = window->GetBounds();
  int left, top, width, height;
  if (bounds_dict->GetInteger("left", &left)) {
    bounds.set_x(left);
    set_bounds = true;
  }
  if (bounds_dict->GetInteger("top", &top)) {
    bounds.set_y(top);
    set_bounds = true;
  }
  if (bounds_dict->GetInteger("width", &width)) {
    if (width < 0)
      return DevToolsProtocol::CreateInvalidParamsResponse(id, "width");
    bounds.set_width(width);
    set_bounds = true;
  }
  if (bounds_dict->GetInteger("height", &height)) {
    if (height < 0)
      return DevToolsProtocol::CreateInvalidParamsResponse(id, "height");
    bounds.set_height(height);
    set_bounds = true;
  }

  if (set_bounds && window_state != "normal") {
    return DevToolsProtocol::CreateErrorResponse(
        id,
        "The 'minimized', 'maximized' and 'fullscreen' states cannot be "
        "combined with 'left', 'top', 'width' or 'height'");
  }

  if (set_bounds && (window->IsMinimized() || window->IsMaximized() ||
                     window->IsFullscreen())) {
    return DevToolsProtocol::CreateErrorResponse(
        id,
        "To resize minimized/maximized/fullscreen window, restore it to normal "
        "state first.");
  }

  if (window_state == "fullscreen") {
    if (window->IsMinimized()) {
      return DevToolsProtocol::CreateErrorResponse(id,
                                                   "To make minimized window "
                                                   "fullscreen, restore it to "
                                                   "normal state first.");
    }
    window->GetExclusiveAccessContext()->EnterFullscreen(
        GURL(), EXCLUSIVE_ACCESS_BUBBLE_TYPE_NONE);
  }

  if (window_state == "maximized") {
    if (window->IsMinimized() || window->IsFullscreen()) {
      return DevToolsProtocol::CreateErrorResponse(
          id,
          "To maximize a minimized or fullscreen window, restore it to normal "
          "state first.");
    }
    window->Maximize();
  }

  if (window_state == "minimized") {
    if (window->IsFullscreen()) {
      return DevToolsProtocol::CreateErrorResponse(
          id,
          "To minimize a fullscreen window, restore it to normal "
          "state first.");
    }
    window->Minimize();
  }

  if (window_state == "normal") {
    if (window->IsFullscreen()) {
      window->GetExclusiveAccessContext()->ExitFullscreen();
    } else if (window->IsMinimized()) {
      window->Show();
    } else if (window->IsMaximized()) {
      window->Restore();
    } else if (set_bounds) {
      window->SetBounds(bounds);
    }
  }

  return DevToolsProtocol::CreateSuccessResponse(id, nullptr);
}

}  // namespace

DevToolsBrowserProtocolHandler::DevToolsBrowserProtocolHandler() {}

DevToolsBrowserProtocolHandler::~DevToolsBrowserProtocolHandler() {}

std::unique_ptr<base::DictionaryValue>
DevToolsBrowserProtocolHandler::HandleCommand(
    int id,
    const std::string& method,
    const base::DictionaryValue& params) {
  if (method == chrome::devtools::Browser::getWindowForTarget::kName)
    return GetWindowForTarget(id, params);
  if (method == chrome::devtools::Browser::getWindowBounds::kName)
    return GetWindowBounds(id, params);
  if (method == chrome::devtools::Browser::setWindowBounds::kName)
    return SetWindowBounds(id, params);
  return nullptr;
}

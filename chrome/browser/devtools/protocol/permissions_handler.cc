// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/protocol/permissions_handler.h"

#include "components/content_settings/core/common/content_settings.h"

namespace {

}  // namespace

PermissionsHandler::PermissionsHandler(protocol::UberDispatcher* dispatcher) {
  // Dispatcher can be null in tests.
  if (dispatcher)
    protocol::Permissions::Dispatcher::wire(dispatcher, this);
}

PermissionsHandler::~PermissionsHandler() = default;

protocol::Response PermissionsHandler::SetPermission(
    ContentSettingsType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const base::Callback<void(ContentSetting)>& callback) {
  auto host = content::DevToolsAgentHost::GetForId(target_id);
  if (!host)
    return protocol::Response::Error("No target with given id");
  content::WebContents* web_contents = host->GetWebContents();
  if (!web_contents)
    return protocol::Response::Error("No web contents in the target");

  Browser* browser = nullptr;
  for (auto* b : *BrowserList::GetInstance()) {
    int tab_index = b->tab_strip_model()->GetIndexOfWebContents(web_contents);
    if (tab_index != TabStripModel::kNoTab)
      browser = b;
  }
  if (!browser)
    return protocol::Response::Error("Browser window not found");

  BrowserWindow* window = browser->window();
  *out_window_id = browser->session_id().id();
  *out_bounds = GetBrowserWindowBounds(window);
  return protocol::Response::OK();
}

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_NTP_APP_ICON_WEBUI_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_NTP_APP_ICON_WEBUI_HANDLER_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/task/cancelable_task_tracker.h"
#include "content/public/browser/web_ui_message_handler.h"

class AppIconColorManager;

namespace base {
class ListValue;
}

class AppIconWebUIHandler : public content::WebUIMessageHandler {
 public:
  AppIconWebUIHandler();
  ~AppIconWebUIHandler() override;

  // WebUIMessageHandler
  void RegisterMessages() override;

  // As above, but for an app tile. The sole argument is the extension ID.
  void HandleGetAppIconDominantColor(const base::ListValue* args);

  // Callback getting signal that an app icon is loaded.
  void NotifyAppIconReady(const std::string& extension_id);

 private:
  base::CancelableTaskTracker cancelable_task_tracker_;

  // Manage retrieval of icons from apps.
  std::unique_ptr<AppIconColorManager> app_icon_color_manager_;

  DISALLOW_COPY_AND_ASSIGN(AppIconWebUIHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_NTP_APP_ICON_WEBUI_HANDLER_H_

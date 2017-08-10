// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_WEBUI_SAFE_BROWSING_UI_H_
#define COMPONENTS_SAFE_BROWSING_WEBUI_SAFE_BROWSING_UI_H_

#include "base/macros.h"
//#include "components/safe_browsing/web_ui/threat_details_router.h"
#include "components/safe_browsing/web_ui/webui.pb.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace base {
class ListValue;
}

namespace safe_browsing {
class SafeBrowsingUIHandler : public content::WebUIMessageHandler {
 public:
  SafeBrowsingUIHandler(content::BrowserContext*);
  ~SafeBrowsingUIHandler() override;

  void GetExperiments(const base::ListValue* args);
  void GetPrefs(const base::ListValue* args);
  void GetDatabaseManagerInfo(const base::ListValue* args);
  void GetThreatDetails(const base::ListValue* args);
  void GetThreatDetails1(const std::string& text) const;

  void RegisterMessages() override;

 private:
  content::BrowserContext* browser_context_;
  // If the reciever is currently registered, unregisters |this|.
  void UnregisterThreatDetailsReceiverIfNecessary();

  // Whether |this| registered as a threat details receiver.
  bool registered_as_thread_details_receiver_;

  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingUIHandler);
};

// The WebUI for chrome://safe-browsing
class SafeBrowsingUI : public content::WebUIController {
 public:
  explicit SafeBrowsingUI(content::WebUI* web_ui);
  ~SafeBrowsingUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingUI);
};

}  // namespace safe_browsing

#endif  // COMPONENTS_SAFE_BROWSING_WEBUI_SAFE_BROWSING_UI_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_WEBUI_SAFE_BROWSING_UI_H_
#define COMPONENTS_SAFE_BROWSING_WEBUI_SAFE_BROWSING_UI_H_

#include "base/macros.h"
#include "components/safe_browsing/proto/csd.pb.h"
#include "components/safe_browsing/proto/webui.pb.h"
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
  enum ThreatDetailsOperation {
    ADD = 0,
    DELETE = 1,
    GET = 2,
  };
  // Get the experiments that are currently running for the user.
  void GetExperiments(const base::ListValue* args);
  // Get the Safe Browsing related preferences.
  void GetPrefs(const base::ListValue* args);
  // Get the information related to Safe Browsing databases.
  void GetDatabaseManagerInfo(const base::ListValue* args);
  // Get the ThreatDetails that have been collected since at least one tab of
  // WebUI was opened.
  void GetSentThreatDetails(const base::ListValue* args);
  // Get the new ThreatDetails messages sent while one or more WebUI tabs are
  // opened.
  void GetThreatDetailsUpdate(ClientSafeBrowsingReportRequest* threat_detail);

  // Register callbacks for WebUI messages.
  void RegisterMessages() override;

  // Record and show new ThreatDetails when at least one WebUI tab is opened.
  static void AddNewThreatDetails(
      std::unique_ptr<ClientSafeBrowsingReportRequest> threat_detail);
  // Records, updates, or deletes ThreatDetails, depending on the
  // ThreatDetailsOperation value.
  static const std::vector<std::unique_ptr<ClientSafeBrowsingReportRequest>>&
  GetOrUpdateThreatDetails(
      ThreatDetailsOperation mode,
      std::unique_ptr<ClientSafeBrowsingReportRequest> threat_detail);

 private:
  content::BrowserContext* browser_context_;
  // List that keeps all the WebUI listener objects.
  static std::vector<SafeBrowsingUIHandler*> webui_list_;
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

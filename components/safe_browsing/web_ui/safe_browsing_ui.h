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
    // Add the new message in the list of sent ClientSafeBrowsingReportRequest
    // messages in GetOrUpdateThreatDetails and send it to all the
    // chrome://safe-browsing opened tabs.
    ADD = 0,
    // Delete the list of the sent ClientSafeBrowsingReportRequest messages.
    DELETE = 1,
    // Get the list of the sent ClientSafeBrowsingReportRequest messages.
    GET = 2,
  };

  // Get the experiments that are currently enabled per Chrome instance.
  void GetExperiments(const base::ListValue* args);

  // Get the Safe Browsing related preferences for the current user.
  void GetPrefs(const base::ListValue* args);

  // Get the information related to the Safe Browsing database and full hash
  // cache.
  void GetDatabaseManagerInfo(const base::ListValue* args);

  // Get the ThreatDetails that have been collected since the oldest currently
  // open chrome://safe-browsing tab was opened.
  void GetSentThreatDetails(const base::ListValue* args);

  // Get the new ThreatDetails messages sent from ThreatDetails when a ping is
  // sent, while one or more WebUI tabs are opened.
  void GetThreatDetailsUpdate(ClientSafeBrowsingReportRequest* threat_detail);

  // Register callbacks for WebUI messages.
  void RegisterMessages() override;

  // Record and show new pings sent from ThreatDetail, when at least one WebUI
  // tab is opened. Called from the Javascript when the page is created.
  static void AddNewThreatDetails(
      std::unique_ptr<ClientSafeBrowsingReportRequest> threat_detail);
  // Records, updates, or deletes the list of sent
  // ClientSafeBrowsingReportRequest messages, depending on the
  // ThreatDetailsOperation value. The function holds the static list of all the
  // sent messages, which exists until the last chrome://safe-browsing WebUI
  // object is destroyed.
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

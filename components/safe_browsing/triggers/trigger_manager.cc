// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/triggers/trigger_manager.h"

#include "components/safe_browsing/base_ui_manager.h"
#include "components/safe_browsing/browser/threat_details.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/security_interstitials/content/unsafe_resource.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request_context_getter.h"

namespace safe_browsing {

namespace {

bool CanStartDataCollection(const PrefService& pref_service,
                            const content::WebContents& web_contents) {
  // We start data collection as long as user is not incognito and is able to
  // change the Extended Reporting opt-in. We don't require them to be opted-in
  // to SBER to begin collecting data, since they may be able to change the
  // setting while data collection is running (eg: on a security interstitial).
  return !web_contents.GetBrowserContext()->IsOffTheRecord() &&
         IsExtendedReportingOptInAllowed(pref_service);
}

bool CanSendReport(const PrefService& pref_service,
                   const content::WebContents& web_contents) {
  // Reports are only sent for non-incoginito users who are allowed to modify
  // the Extended Reporting setting and have opted-in to Extended Reporting.
  return !web_contents.GetBrowserContext()->IsOffTheRecord() &&
         IsExtendedReportingOptInAllowed(pref_service) &&
         IsExtendedReportingEnabled(pref_service);
}

}  // namespace

TriggerManager::TriggerManager(BaseUIManager* ui_manager)
    : ui_manager_(ui_manager) {}

TriggerManager::~TriggerManager() {}

bool TriggerManager::StartCollectingThreatDetails(
    content::WebContents* web_contents,
    const security_interstitials::UnsafeResource& resource,
    net::URLRequestContextGetter* request_context_getter,
    history::HistoryService* history_service,
    const PrefService& pref_service) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!CanStartDataCollection(pref_service, *web_contents))
    return false;

  // Ensure we're not already collecting data on this tab.
  if (base::ContainsKey(data_collectors_map_, web_contents))
    return false;

  data_collectors_map_[web_contents] = scoped_refptr<ThreatDetails>(
      ThreatDetails::NewThreatDetails(ui_manager_, web_contents, resource,
                                      request_context_getter, history_service));
  return true;
}

bool TriggerManager::FinishCollectingThreatDetails(
    content::WebContents* web_contents,
    const base::TimeDelta& delay,
    bool did_proceed,
    int num_visits,
    const PrefService& pref_service) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Make sure there's a data collector running on this tab.
  if (!base::ContainsKey(data_collectors_map_, web_contents))
    return false;

  // Determine whether a report should be sent.
  bool should_send_report = CanSendReport(pref_service, *web_contents);

  // Find the data collector and tell it to finish collecting data, and then
  // remove it from our map. We release ownership of the data collector here but
  // it will live until the end of the FinishCollection call because it
  // implements RefCountedThreadSafe.
  if (should_send_report) {
    scoped_refptr<ThreatDetails> threat_details =
        data_collectors_map_[web_contents];
    content::BrowserThread::PostDelayedTask(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ThreatDetails::FinishCollection, threat_details,
                       did_proceed, num_visits),
        delay);
  }

  // Regardless of whether the report got sent, clean up the data collector on
  // this tab.
  data_collectors_map_.erase(web_contents);

  return should_send_report;
}

}  // namespace safe_browsing

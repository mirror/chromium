// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_DOWNLOAD_PROTECTION_DOWNLOAD_URL_SB_CLIENT_H_
#define CHROME_BROWSER_SAFE_BROWSING_DOWNLOAD_PROTECTION_DOWNLOAD_URL_SB_CLIENT_H_

#include "chrome/browser/safe_browsing/download_protection/download_check_enums.h"

#include <stddef.h>

#include <memory>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/format_macros.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/sparse_histogram.h"
#include "base/scoped_observer.h"
#include "base/sequenced_task_runner_helpers.h"
#include "base/sha1.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/safe_browsing/download_feedback_service.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/safe_browsing/archive_analyzer_results.h"
#include "chrome/common/safe_browsing/download_protection_util.h"
#include "chrome/common/safe_browsing/file_type_policies.h"
#include "chrome/common/url_constants.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/google/core/browser/google_util.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/safe_browsing/common/safebrowsing_switches.h"
#include "components/safe_browsing/common/utils.h"
#include "components/safe_browsing/csd.pb.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/page_navigator.h"
#include "google_apis/google_api_keys.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "net/cert/x509_cert_types.h"
#include "net/cert/x509_certificate.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_request_status.h"

using content::BrowserThread;

namespace safe_browsing {

// TODO this is copied from download_protection_service
const void* const kDownloadReferrerChainDataKey =
    &kDownloadReferrerChainDataKey;

class DownloadUrlSBClient
    : public SafeBrowsingDatabaseManager::Client,
      public content::DownloadItem::Observer,
      public base::RefCountedThreadSafe<DownloadUrlSBClient,
                                        BrowserThread::DeleteOnUIThread> {
 public:
  DownloadUrlSBClient(
      content::DownloadItem* item,
      DownloadProtectionService* service,
      const CheckDownloadCallback& callback,
      const scoped_refptr<SafeBrowsingUIManager>& ui_manager,
      const scoped_refptr<SafeBrowsingDatabaseManager>& database_manager);

  // Implements DownloadItem::Observer.
  void OnDownloadDestroyed(content::DownloadItem* download) override;

  void StartCheck();

  bool IsDangerous(SBThreatType threat_type) const;

  // Implements SafeBrowsingDatabaseManager::Client.
  void OnCheckDownloadUrlResult(const std::vector<GURL>& url_chain,
                                SBThreatType threat_type) override;

 private:
  friend class base::RefCountedThreadSafe<DownloadUrlSBClient>;
  friend struct BrowserThread::DeleteOnThread<BrowserThread::UI>;
  friend class base::DeleteHelper<DownloadUrlSBClient>;

  ~DownloadUrlSBClient() override;

  void CheckDone(SBThreatType threat_type);

  void ReportMalware(SBThreatType threat_type);

  void IdentifyReferrerChain();

  void UpdateDownloadCheckStats(SBStatsType stat_type);

  // The DownloadItem we are checking. Must be accessed only on UI thread.
  content::DownloadItem* item_;
  // Copies of data from |item_| for access on other threads.
  std::string sha256_hash_;
  std::vector<GURL> url_chain_;
  GURL referrer_url_;
  DownloadProtectionService* service_;
  CheckDownloadCallback callback_;
  scoped_refptr<SafeBrowsingUIManager> ui_manager_;
  base::TimeTicks start_time_;
  const SBStatsType total_type_;
  const SBStatsType dangerous_type_;
  ExtendedReportingLevel extended_reporting_level_;
  scoped_refptr<SafeBrowsingDatabaseManager> database_manager_;
  ScopedObserver<content::DownloadItem, content::DownloadItem::Observer>
      download_item_observer_;

  DISALLOW_COPY_AND_ASSIGN(DownloadUrlSBClient);
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_DOWNLOAD_PROTECTION_DOWNLOAD_URL_SB_CLIENT_H_

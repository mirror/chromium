// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/download_protection/download_url_sb_client.h"

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
#include "chrome/browser/safe_browsing/download_protection/download_feedback_service.h"
#include "chrome/browser/safe_browsing/download_protection/download_protection_util.h"
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

namespace safe_browsing {

DownloadUrlSBClient::DownloadUrlSBClient(
    content::DownloadItem* item,
    DownloadProtectionService* service,
    const CheckDownloadCallback& callback,
    const scoped_refptr<SafeBrowsingUIManager>& ui_manager,
    const scoped_refptr<SafeBrowsingDatabaseManager>& database_manager)
    : item_(item),
      sha256_hash_(item->GetHash()),
      url_chain_(item->GetUrlChain()),
      referrer_url_(item->GetReferrerUrl()),
      service_(service),
      callback_(callback),
      ui_manager_(ui_manager),
      start_time_(base::TimeTicks::Now()),
      total_type_(DOWNLOAD_URL_CHECKS_TOTAL),
      dangerous_type_(DOWNLOAD_URL_CHECKS_MALWARE),
      database_manager_(database_manager),
      download_item_observer_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(item_);
  DCHECK(service_);
  download_item_observer_.Add(item_);
  Profile* profile = Profile::FromBrowserContext(item_->GetBrowserContext());
  extended_reporting_level_ =
      profile ? GetExtendedReportingLevel(*profile->GetPrefs())
              : SBER_LEVEL_OFF;
}

// Implements DownloadItem::Observer.
void DownloadUrlSBClient::OnDownloadDestroyed(content::DownloadItem* download) {
  download_item_observer_.Remove(item_);
  item_ = nullptr;
}

void DownloadUrlSBClient::StartCheck() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!database_manager_.get() ||
      database_manager_->CheckDownloadUrl(url_chain_, this)) {
    CheckDone(SB_THREAT_TYPE_SAFE);
  } else {
    // Add a reference to this object to prevent it from being destroyed
    // before url checking result is returned.
    AddRef();
  }
}

bool DownloadUrlSBClient::IsDangerous(SBThreatType threat_type) const {
  return threat_type == SB_THREAT_TYPE_URL_BINARY_MALWARE;
}

// Implements SafeBrowsingDatabaseManager::Client.
void DownloadUrlSBClient::OnCheckDownloadUrlResult(
    const std::vector<GURL>& url_chain,
    SBThreatType threat_type) {
  CheckDone(threat_type);
  UMA_HISTOGRAM_TIMES("SB2.DownloadUrlCheckDuration",
                      base::TimeTicks::Now() - start_time_);
  Release();
}

DownloadUrlSBClient::~DownloadUrlSBClient() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void DownloadUrlSBClient::CheckDone(SBThreatType threat_type) {
  DownloadCheckResult result = IsDangerous(threat_type) ? DANGEROUS : SAFE;
  UpdateDownloadCheckStats(total_type_);
  if (threat_type != SB_THREAT_TYPE_SAFE) {
    UpdateDownloadCheckStats(dangerous_type_);
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&DownloadUrlSBClient::ReportMalware, this, threat_type));
  } else {
    // Identify download referrer chain, which will be used in
    // ClientDownloadRequest.
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&DownloadUrlSBClient::IdentifyReferrerChain, this));
  }
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::BindOnce(callback_, result));
}

void DownloadUrlSBClient::ReportMalware(SBThreatType threat_type) {
  std::string post_data;
  if (!sha256_hash_.empty()) {
    post_data +=
        base::HexEncode(sha256_hash_.data(), sha256_hash_.size()) + "\n";
  }
  for (size_t i = 0; i < url_chain_.size(); ++i) {
    post_data += url_chain_[i].spec() + "\n";
  }

  safe_browsing::HitReport hit_report;
  hit_report.malicious_url = url_chain_.back();
  hit_report.page_url = url_chain_.front();
  hit_report.referrer_url = referrer_url_;
  hit_report.is_subresource = true;
  hit_report.threat_type = threat_type;
  // TODO(nparker) Replace this with database_manager_->GetThreatSource();
  hit_report.threat_source = safe_browsing::ThreatSource::LOCAL_PVER3;
  // TODO(nparker) Populate hit_report.population_id once Pver4 is used here.
  hit_report.post_data = post_data;
  hit_report.extended_reporting_level = extended_reporting_level_;
  hit_report.is_metrics_reporting_active =
      ChromeMetricsServiceAccessor::IsMetricsAndCrashReportingEnabled();

  ui_manager_->MaybeReportSafeBrowsingHit(hit_report, item_->GetWebContents());
}

void DownloadUrlSBClient::IdentifyReferrerChain() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!item_)
    return;

  item_->SetUserData(kDownloadReferrerChainDataKey,
                     base::MakeUnique<ReferrerChainData>(
                         service_->IdentifyReferrerChain(*item_)));
}

void DownloadUrlSBClient::UpdateDownloadCheckStats(SBStatsType stat_type) {
  UMA_HISTOGRAM_ENUMERATION("SB2.DownloadChecks", stat_type,
                            DOWNLOAD_CHECKS_MAX);
}

}  // namespace safe_browsing

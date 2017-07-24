// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/download_protection/download_protection_service.h"

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

namespace {

const int64_t kDownloadRequestTimeoutMs = 7000;
// We sample 1% of whitelisted downloads to still send out download pings.
const double kWhitelistDownloadSampleRate = 0.01;

// The number of user gestures we trace back for download attribution.
const int kDownloadAttributionUserGestureLimit = 2;

const void* const kDownloadReferrerChainDataKey =
    &kDownloadReferrerChainDataKey;

enum WhitelistType {
  NO_WHITELIST_MATCH,
  URL_WHITELIST,
  SIGNATURE_WHITELIST,
  WHITELIST_TYPE_MAX
};

void RecordCountOfWhitelistedDownload(WhitelistType type) {
  UMA_HISTOGRAM_ENUMERATION("SBClientDownload.CheckWhitelistResult", type,
                            WHITELIST_TYPE_MAX);
}

// Enumerate for histogramming purposes.
// DO NOT CHANGE THE ORDERING OF THESE VALUES (different histogram data will
// be mixed together based on their values).
enum SBStatsType {
  DOWNLOAD_URL_CHECKS_TOTAL,
  DOWNLOAD_URL_CHECKS_CANCELED,
  DOWNLOAD_URL_CHECKS_MALWARE,

  DOWNLOAD_HASH_CHECKS_TOTAL,
  DOWNLOAD_HASH_CHECKS_MALWARE,

  // Memory space for histograms is determined by the max.
  // ALWAYS ADD NEW VALUES BEFORE THIS ONE.
  DOWNLOAD_CHECKS_MAX
};

void AddEventUrlToReferrerChain(const content::DownloadItem& item,
                                ReferrerChain* out_referrer_chain) {
  ReferrerChainEntry* event_url_entry = out_referrer_chain->Add();
  event_url_entry->set_url(item.GetURL().spec());
  event_url_entry->set_type(ReferrerChainEntry::EVENT_URL);
  event_url_entry->set_referrer_url(
      item.GetWebContents()->GetLastCommittedURL().spec());
  event_url_entry->set_is_retargeting(false);
  event_url_entry->set_navigation_time_msec(base::Time::Now().ToJavaTime());
  for (const GURL& url : item.GetUrlChain())
    event_url_entry->add_server_redirect_chain()->set_url(url.spec());
}

}  // namespace

const char DownloadProtectionService::kDownloadRequestUrl[] =
    "https://sb-ssl.google.com/safebrowsing/clientreport/download";

const void* const DownloadProtectionService::kDownloadPingTokenKey =
    &kDownloadPingTokenKey;

// SafeBrowsing::Client class used to lookup the bad binary URL list.

class DownloadUrlSBClient
    : public SafeBrowsingDatabaseManager::Client,
      public content::DownloadItem::Observer,
      public base::RefCountedThreadSafe<DownloadUrlSBClient,
                                        BrowserThread::DeleteOnUIThread> {
 public:
  DownloadUrlSBClient(
      content::DownloadItem* item,
      DownloadProtectionService* service,
      const DownloadProtectionService::CheckDownloadCallback& callback,
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
  void OnDownloadDestroyed(content::DownloadItem* download) override {
    download_item_observer_.Remove(item_);
    item_ = nullptr;
  }

  void StartCheck() {
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

  bool IsDangerous(SBThreatType threat_type) const {
    return threat_type == SB_THREAT_TYPE_URL_BINARY_MALWARE;
  }

  // Implements SafeBrowsingDatabaseManager::Client.
  void OnCheckDownloadUrlResult(const std::vector<GURL>& url_chain,
                                SBThreatType threat_type) override {
    CheckDone(threat_type);
    UMA_HISTOGRAM_TIMES("SB2.DownloadUrlCheckDuration",
                        base::TimeTicks::Now() - start_time_);
    Release();
  }

 private:
  friend class base::RefCountedThreadSafe<DownloadUrlSBClient>;
  friend struct BrowserThread::DeleteOnThread<BrowserThread::UI>;
  friend class base::DeleteHelper<DownloadUrlSBClient>;

  ~DownloadUrlSBClient() override { DCHECK_CURRENTLY_ON(BrowserThread::UI); }

  void CheckDone(SBThreatType threat_type) {
    DownloadCheckEnums::DownloadCheckResult result =
        IsDangerous(threat_type) ? DownloadCheckEnums::DANGEROUS
                                 : DownloadCheckEnums::SAFE;
    UpdateDownloadCheckStats(total_type_);
    if (threat_type != SB_THREAT_TYPE_SAFE) {
      UpdateDownloadCheckStats(dangerous_type_);
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::BindOnce(&DownloadUrlSBClient::ReportMalware, this,
                         threat_type));
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

  void ReportMalware(SBThreatType threat_type) {
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

    ui_manager_->MaybeReportSafeBrowsingHit(hit_report,
                                            item_->GetWebContents());
  }

  void IdentifyReferrerChain() {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    if (!item_)
      return;

    item_->SetUserData(kDownloadReferrerChainDataKey,
                       base::MakeUnique<ReferrerChainData>(
                           service_->IdentifyReferrerChain(*item_)));
  }

  void UpdateDownloadCheckStats(SBStatsType stat_type) {
    UMA_HISTOGRAM_ENUMERATION("SB2.DownloadChecks", stat_type,
                              DOWNLOAD_CHECKS_MAX);
  }

  // The DownloadItem we are checking. Must be accessed only on UI thread.
  content::DownloadItem* item_;
  // Copies of data from |item_| for access on other threads.
  std::string sha256_hash_;
  std::vector<GURL> url_chain_;
  GURL referrer_url_;
  DownloadProtectionService* service_;
  DownloadProtectionService::CheckDownloadCallback callback_;
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

// A request for checking whether a PPAPI initiated download is safe.
//
// These are considered different from DownloadManager mediated downloads
// because:
//
// * The download bytes are produced by the PPAPI plugin *after* the check
//   returns due to architectural constraints.
//
// * Since the download bytes are produced by the PPAPI plugin, there's no
//   reliable network request information to associate with the download.
//
// PPAPIDownloadRequest objects are owned by the DownloadProtectionService
// indicated by |service|.
class DownloadProtectionService::PPAPIDownloadRequest
    : public net::URLFetcherDelegate {
 public:
  // The outcome of the request. These values are used for UMA. New values
  // should only be added at the end.
  enum class RequestOutcome : int {
    UNKNOWN,
    REQUEST_DESTROYED,
    UNSUPPORTED_FILE_TYPE,
    TIMEDOUT,
    WHITELIST_HIT,
    REQUEST_MALFORMED,
    FETCH_FAILED,
    RESPONSE_MALFORMED,
    SUCCEEDED
  };

  PPAPIDownloadRequest(
      const GURL& requestor_url,
      const GURL& initiating_frame_url,
      content::WebContents* web_contents,
      const base::FilePath& default_file_path,
      const std::vector<base::FilePath::StringType>& alternate_extensions,
      Profile* profile,
      const CheckDownloadCallback& callback,
      DownloadProtectionService* service,
      scoped_refptr<SafeBrowsingDatabaseManager> database_manager)
      : requestor_url_(requestor_url),
        initiating_frame_url_(initiating_frame_url),
        initiating_main_frame_url_(
            web_contents ? web_contents->GetLastCommittedURL() : GURL()),
        tab_id_(SessionTabHelper::IdForTab(web_contents)),
        default_file_path_(default_file_path),
        alternate_extensions_(alternate_extensions),
        callback_(callback),
        service_(service),
        database_manager_(database_manager),
        start_time_(base::TimeTicks::Now()),
        supported_path_(
            GetSupportedFilePath(default_file_path, alternate_extensions)),
        weakptr_factory_(this) {
    DCHECK(profile);
    is_extended_reporting_ = IsExtendedReportingEnabled(*profile->GetPrefs());

    if (service->navigation_observer_manager()) {
      has_user_gesture_ =
          service->navigation_observer_manager()->HasUserGesture(web_contents);
      if (has_user_gesture_) {
        service->navigation_observer_manager()->OnUserGestureConsumed(
            web_contents, base::Time::Now());
      }
    }
  }

  ~PPAPIDownloadRequest() override {
    if (fetcher_ && !callback_.is_null())
      Finish(RequestOutcome::REQUEST_DESTROYED, DownloadCheckEnums::UNKNOWN);
  }

  // Start the process of checking the download request. The callback passed as
  // the |callback| parameter to the constructor will be invoked with the result
  // of the check at some point in the future.
  //
  // From the this point on, the code is arranged to follow the most common
  // workflow.
  //
  // Note that |this| should be added to the list of pending requests in the
  // associated DownloadProtectionService object *before* calling Start().
  // Otherwise a synchronous Finish() call may result in leaking the
  // PPAPIDownloadRequest object. This is enforced via a DCHECK in
  // DownloadProtectionService.
  void Start() {
    DVLOG(2) << "Starting SafeBrowsing download check for PPAPI download from "
             << requestor_url_ << " for [" << default_file_path_.value() << "] "
             << "supported path is [" << supported_path_.value() << "]";

    if (supported_path_.empty()) {
      // Neither the default_file_path_ nor any path resulting of trying out
      // |alternate_extensions_| are supported by SafeBrowsing.
      Finish(RequestOutcome::UNSUPPORTED_FILE_TYPE, DownloadCheckEnums::SAFE);
      return;
    }

    // In case the request take too long, the check will abort with an UNKNOWN
    // verdict. The weak pointer used for the timeout will be invalidated (and
    // hence would prevent the timeout) if the check completes on time and
    // execution reaches Finish().
    BrowserThread::PostDelayedTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&PPAPIDownloadRequest::OnRequestTimedOut,
                       weakptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(
            service_->download_request_timeout_ms()));

    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&PPAPIDownloadRequest::CheckWhitelistsOnIOThread,
                       requestor_url_, database_manager_,
                       weakptr_factory_.GetWeakPtr()));
  }

 private:
  // Whitelist checking needs to the done on the IO thread.
  static void CheckWhitelistsOnIOThread(
      const GURL& requestor_url,
      scoped_refptr<SafeBrowsingDatabaseManager> database_manager,
      base::WeakPtr<PPAPIDownloadRequest> download_request) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    DVLOG(2) << " checking whitelists for requestor URL:" << requestor_url;

    bool url_was_whitelisted =
        requestor_url.is_valid() && database_manager &&
        database_manager->MatchDownloadWhitelistUrl(requestor_url);
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&PPAPIDownloadRequest::WhitelistCheckComplete,
                       download_request, url_was_whitelisted));
  }

  void WhitelistCheckComplete(bool was_on_whitelist) {
    DVLOG(2) << __func__ << " was_on_whitelist:" << was_on_whitelist;
    if (was_on_whitelist) {
      RecordCountOfWhitelistedDownload(URL_WHITELIST);
      // TODO(asanka): Should sample whitelisted downloads based on
      // service_->whitelist_sample_rate(). http://crbug.com/610924
      Finish(RequestOutcome::WHITELIST_HIT, DownloadCheckEnums::SAFE);
      return;
    }

    // Not on whitelist, so we are going to check with the SafeBrowsing
    // backend.
    SendRequest();
  }

  void SendRequest() {
    DVLOG(2) << __func__;
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    ClientDownloadRequest request;
    auto population = is_extended_reporting_
                          ? ChromeUserPopulation::EXTENDED_REPORTING
                          : ChromeUserPopulation::SAFE_BROWSING;
    request.mutable_population()->set_user_population(population);
    request.set_download_type(ClientDownloadRequest::PPAPI_SAVE_REQUEST);
    ClientDownloadRequest::Resource* resource = request.add_resources();
    resource->set_type(ClientDownloadRequest::PPAPI_DOCUMENT);
    resource->set_url(requestor_url_.spec());
    request.set_url(requestor_url_.spec());
    request.set_file_basename(supported_path_.BaseName().AsUTF8Unsafe());
    request.set_length(0);
    request.mutable_digests()->set_md5(std::string());
    for (const auto& alternate_extension : alternate_extensions_) {
      if (alternate_extension.empty())
        continue;
      DCHECK_EQ(base::FilePath::kExtensionSeparator, alternate_extension[0]);
      *(request.add_alternate_extensions()) =
          base::FilePath(alternate_extension).AsUTF8Unsafe();
    }
    if (supported_path_ != default_file_path_) {
      *(request.add_alternate_extensions()) =
          base::FilePath(default_file_path_.FinalExtension()).AsUTF8Unsafe();
    }

    service_->AddReferrerChainToPPAPIClientDownloadRequest(
        initiating_frame_url_, initiating_main_frame_url_, tab_id_,
        has_user_gesture_, &request);

    if (!request.SerializeToString(&client_download_request_data_)) {
      // More of an internal error than anything else. Note that the UNKNOWN
      // verdict gets interpreted as "allowed".
      Finish(RequestOutcome::REQUEST_MALFORMED, DownloadCheckEnums::UNKNOWN);
      return;
    }

    service_->ppapi_download_request_callbacks_.Notify(&request);
    DVLOG(2) << "Sending a PPAPI download request for URL: " << request.url();

    net::NetworkTrafficAnnotationTag traffic_annotation =
        net::DefineNetworkTrafficAnnotation("ppapi_download_request", R"(
          semantics {
            sender: "Download Protection Service"
            description:
              "Chromium checks whether a given PPAPI download is likely to be "
              "dangerous by sending this client download request to Google's "
              "Safe Browsing servers. Safe Browsing server will respond to "
              "this request by sending back a verdict, indicating if this "
              "download is safe or the danger type of this download (e.g. "
              "dangerous content, uncommon content, potentially harmful, etc)."
            trigger:
              "When user triggers a non-whitelisted PPAPI download, and the "
              "file extension is supported by download protection service. "
              "Please refer to https://cs.chromium.org/chromium/src/chrome/"
              "browser/resources/safe_browsing/download_file_types.asciipb for "
              "the complete list of supported files."
            data:
              "Download's URL, its referrer chain, and digest. Please refer to "
              "ClientDownloadRequest message in https://cs.chromium.org/"
              "chromium/src/components/safe_browsing/csd.proto for all "
              "submitted features."
            destination: GOOGLE_OWNED_SERVICE
          }
          policy {
            cookies_allowed: YES
            cookies_store: "Safe Browsing cookies store"
            setting:
              "Users can enable or disable the entire Safe Browsing service in "
              "Chromium's settings by toggling 'Protect you and your device "
              "from dangerous sites' under Privacy. This feature is enabled by "
              "default."
            chrome_policy {
              SafeBrowsingEnabled {
                policy_options {mode: MANDATORY}
                SafeBrowsingEnabled: false
              }
            }
          })");
    fetcher_ = net::URLFetcher::Create(0, GetDownloadRequestUrl(),
                                       net::URLFetcher::POST, this,
                                       traffic_annotation);
    data_use_measurement::DataUseUserData::AttachToFetcher(
        fetcher_.get(), data_use_measurement::DataUseUserData::SAFE_BROWSING);
    fetcher_->SetLoadFlags(net::LOAD_DISABLE_CACHE);
    fetcher_->SetAutomaticallyRetryOn5xx(false);
    fetcher_->SetRequestContext(service_->request_context_getter_.get());
    fetcher_->SetUploadData("application/octet-stream",
                            client_download_request_data_);
    fetcher_->Start();
  }

  // net::URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    if (!source->GetStatus().is_success() ||
        net::HTTP_OK != source->GetResponseCode()) {
      Finish(RequestOutcome::FETCH_FAILED, DownloadCheckEnums::UNKNOWN);
      return;
    }

    ClientDownloadResponse response;
    std::string response_body;
    bool got_data = source->GetResponseAsString(&response_body);
    DCHECK(got_data);

    if (response.ParseFromString(response_body)) {
      Finish(RequestOutcome::SUCCEEDED,
             DownloadCheckResultFromClientDownloadResponse(response.verdict()));
    } else {
      Finish(RequestOutcome::RESPONSE_MALFORMED, DownloadCheckEnums::UNKNOWN);
    }
  }

  void OnRequestTimedOut() {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    DVLOG(2) << __func__;
    Finish(RequestOutcome::TIMEDOUT, DownloadCheckEnums::UNKNOWN);
  }

  void Finish(RequestOutcome reason,
              DownloadCheckEnums::DownloadCheckResult response) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    DVLOG(2) << __func__ << " response: " << response;
    UMA_HISTOGRAM_SPARSE_SLOWLY(
        "SBClientDownload.PPAPIDownloadRequest.RequestOutcome",
        static_cast<int>(reason));
    UMA_HISTOGRAM_SPARSE_SLOWLY("SBClientDownload.PPAPIDownloadRequest.Result",
                                response);
    UMA_HISTOGRAM_TIMES("SBClientDownload.PPAPIDownloadRequest.RequestDuration",
                        start_time_ - base::TimeTicks::Now());
    if (!callback_.is_null())
      base::ResetAndReturn(&callback_).Run(response);
    fetcher_.reset();
    weakptr_factory_.InvalidateWeakPtrs();

    // If the request is being destroyed, don't notify the service_. It already
    // knows.
    if (reason == RequestOutcome::REQUEST_DESTROYED)
      return;

    service_->PPAPIDownloadCheckRequestFinished(this);
    // |this| is deleted.
  }

  static DownloadCheckEnums::DownloadCheckResult
  DownloadCheckResultFromClientDownloadResponse(
      ClientDownloadResponse::Verdict verdict) {
    switch (verdict) {
      case ClientDownloadResponse::SAFE:
        return DownloadCheckEnums::SAFE;
      case ClientDownloadResponse::UNCOMMON:
        return DownloadCheckEnums::UNCOMMON;
      case ClientDownloadResponse::POTENTIALLY_UNWANTED:
        return DownloadCheckEnums::POTENTIALLY_UNWANTED;
      case ClientDownloadResponse::DANGEROUS:
        return DownloadCheckEnums::DANGEROUS;
      case ClientDownloadResponse::DANGEROUS_HOST:
        return DownloadCheckEnums::DANGEROUS_HOST;
      case ClientDownloadResponse::UNKNOWN:
        return DownloadCheckEnums::UNKNOWN;
    }
    return DownloadCheckEnums::UNKNOWN;
  }

  // Given a |default_file_path| and a list of |alternate_extensions|,
  // constructs a FilePath with each possible extension and returns one that
  // satisfies IsCheckedBinaryFile(). If none are supported, returns an
  // empty FilePath.
  static base::FilePath GetSupportedFilePath(
      const base::FilePath& default_file_path,
      const std::vector<base::FilePath::StringType>& alternate_extensions) {
    const FileTypePolicies* file_type_policies =
        FileTypePolicies::GetInstance();
    if (file_type_policies->IsCheckedBinaryFile(default_file_path))
      return default_file_path;

    for (const auto& extension : alternate_extensions) {
      base::FilePath alternative_file_path =
          default_file_path.ReplaceExtension(extension);
      if (file_type_policies->IsCheckedBinaryFile(alternative_file_path))
        return alternative_file_path;
    }

    return base::FilePath();
  }

  std::unique_ptr<net::URLFetcher> fetcher_;
  std::string client_download_request_data_;

  // URL of document that requested the PPAPI download.
  const GURL requestor_url_;

  // URL of the frame that hosts the PPAPI plugin.
  const GURL initiating_frame_url_;

  // URL of the tab that contains the initialting_frame.
  const GURL initiating_main_frame_url_;

  // Tab id that associated with the PPAPI plugin, computed by
  // SessionTabHelper::IdForTab().
  int tab_id_;

  // If the user interacted with this PPAPI plugin to trigger the download.
  bool has_user_gesture_;

  // Default download path requested by the PPAPI plugin.
  const base::FilePath default_file_path_;

  // List of alternate extensions provided by the PPAPI plugin. Each extension
  // must begin with a leading extension separator.
  const std::vector<base::FilePath::StringType> alternate_extensions_;

  // Callback to invoke with the result of the PPAPI download request check.
  CheckDownloadCallback callback_;

  DownloadProtectionService* service_;
  const scoped_refptr<SafeBrowsingDatabaseManager> database_manager_;

  // Time request was started.
  const base::TimeTicks start_time_;

  // A download path that is supported by SafeBrowsing. This is determined by
  // invoking GetSupportedFilePath(). If non-empty,
  // IsCheckedBinaryFile(supported_path_) is always true. This
  // path is therefore used as the download target when sending the SafeBrowsing
  // ping.
  const base::FilePath supported_path_;

  bool is_extended_reporting_;

  base::WeakPtrFactory<PPAPIDownloadRequest> weakptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PPAPIDownloadRequest);
};

DownloadProtectionService::DownloadProtectionService(
    SafeBrowsingService* sb_service)
    : navigation_observer_manager_(nullptr),
      request_context_getter_(sb_service ? sb_service->url_request_context()
                                         : nullptr),
      enabled_(false),
      binary_feature_extractor_(new BinaryFeatureExtractor()),
      download_request_timeout_ms_(kDownloadRequestTimeoutMs),
      feedback_service_(new DownloadFeedbackService(
          request_context_getter_.get(),
          base::CreateSequencedTaskRunnerWithTraits(
              {base::MayBlock(), base::TaskPriority::BACKGROUND})
              .get())),
      whitelist_sample_rate_(kWhitelistDownloadSampleRate) {
  if (sb_service) {
    ui_manager_ = sb_service->ui_manager();
    database_manager_ = sb_service->database_manager();
    navigation_observer_manager_ = sb_service->navigation_observer_manager();
    ParseManualBlacklistFlag();
  }
}

DownloadProtectionService::~DownloadProtectionService() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  CancelPendingRequests();
}

void DownloadProtectionService::SetEnabled(bool enabled) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (enabled == enabled_) {
    return;
  }
  enabled_ = enabled;
  if (!enabled_) {
    CancelPendingRequests();
  }
}

void DownloadProtectionService::ParseManualBlacklistFlag() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(
          safe_browsing::switches::kSbManualDownloadBlacklist))
    return;

  std::string flag_val = command_line->GetSwitchValueASCII(
      safe_browsing::switches::kSbManualDownloadBlacklist);
  for (const std::string& hash_hex : base::SplitString(
           flag_val, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
    std::vector<uint8_t> bytes;
    if (base::HexStringToBytes(hash_hex, &bytes) && bytes.size() == 32) {
      manual_blacklist_hashes_.insert(std::string(bytes.begin(), bytes.end()));
    } else {
      LOG(FATAL) << "Bad sha256 hex value '" << hash_hex << "' found in --"
                 << safe_browsing::switches::kSbManualDownloadBlacklist;
    }
  }
}

bool DownloadProtectionService::IsHashManuallyBlacklisted(
    const std::string& sha256_hash) const {
  return manual_blacklist_hashes_.count(sha256_hash) > 0;
}

void DownloadProtectionService::CheckClientDownload(
    content::DownloadItem* item,
    const CheckDownloadCallback& callback) {
  scoped_refptr<CheckClientDownloadRequest> request(
      new CheckClientDownloadRequest(item, callback, this, database_manager_,
                                     binary_feature_extractor_.get()));
  download_requests_.insert(request);
  request->Start();
}

void DownloadProtectionService::CheckDownloadUrl(
    content::DownloadItem* item,
    const CheckDownloadCallback& callback) {
  DCHECK(!item->GetUrlChain().empty());
  scoped_refptr<DownloadUrlSBClient> client(new DownloadUrlSBClient(
      item, this, callback, ui_manager_, database_manager_));
  // The client will release itself once it is done.
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&DownloadUrlSBClient::StartCheck, client));
}

bool DownloadProtectionService::IsSupportedDownload(
    const content::DownloadItem& item,
    const base::FilePath& target_path) const {
  DownloadCheckEnums::DownloadCheckResultReason reason =
      DownloadCheckEnums::REASON_MAX;
  ClientDownloadRequest::DownloadType type =
      ClientDownloadRequest::WIN_EXECUTABLE;
  // TODO(nparker): Remove the CRX check here once can support
  // UNKNOWN types properly.  http://crbug.com/581044
  return (CheckClientDownloadRequest::IsSupportedDownload(item, target_path,
                                                          &reason, &type) &&
          (ClientDownloadRequest::CHROME_EXTENSION != type));
}

void DownloadProtectionService::CheckPPAPIDownloadRequest(
    const GURL& requestor_url,
    const GURL& initiating_frame_url,
    content::WebContents* web_contents,
    const base::FilePath& default_file_path,
    const std::vector<base::FilePath::StringType>& alternate_extensions,
    Profile* profile,
    const CheckDownloadCallback& callback) {
  DVLOG(1) << __func__ << " url:" << requestor_url
           << " default_file_path:" << default_file_path.value();
  std::unique_ptr<PPAPIDownloadRequest> request(new PPAPIDownloadRequest(
      requestor_url, initiating_frame_url, web_contents, default_file_path,
      alternate_extensions, profile, callback, this, database_manager_));
  PPAPIDownloadRequest* request_copy = request.get();
  auto insertion_result = ppapi_download_requests_.insert(
      std::make_pair(request_copy, std::move(request)));
  DCHECK(insertion_result.second);
  insertion_result.first->second->Start();
}

DownloadProtectionService::ClientDownloadRequestSubscription
DownloadProtectionService::RegisterClientDownloadRequestCallback(
    const ClientDownloadRequestCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return client_download_request_callbacks_.Add(callback);
}

DownloadProtectionService::PPAPIDownloadRequestSubscription
DownloadProtectionService::RegisterPPAPIDownloadRequestCallback(
    const PPAPIDownloadRequestCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return ppapi_download_request_callbacks_.Add(callback);
}

void DownloadProtectionService::CancelPendingRequests() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  for (auto it = download_requests_.begin(); it != download_requests_.end();) {
    // We need to advance the iterator before we cancel because canceling
    // the request will invalidate it when RequestFinished is called below.
    scoped_refptr<CheckClientDownloadRequest> tmp = *it++;
    tmp->Cancel();
  }
  DCHECK(download_requests_.empty());

  // It is sufficient to delete the list of PPAPI download requests.
  ppapi_download_requests_.clear();
}

void DownloadProtectionService::RequestFinished(
    CheckClientDownloadRequest* request) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto it = download_requests_.find(request);
  DCHECK(it != download_requests_.end());
  download_requests_.erase(*it);
}

void DownloadProtectionService::PPAPIDownloadCheckRequestFinished(
    PPAPIDownloadRequest* request) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto it = ppapi_download_requests_.find(request);
  DCHECK(it != ppapi_download_requests_.end());
  ppapi_download_requests_.erase(it);
}

void DownloadProtectionService::ShowDetailsForDownload(
    const content::DownloadItem& item,
    content::PageNavigator* navigator) {
  GURL learn_more_url(chrome::kDownloadScanningLearnMoreURL);
  learn_more_url = google_util::AppendGoogleLocaleParam(
      learn_more_url, g_browser_process->GetApplicationLocale());
  learn_more_url = net::AppendQueryParameter(
      learn_more_url, "ctx",
      base::IntToString(static_cast<int>(item.GetDangerType())));
  navigator->OpenURL(
      content::OpenURLParams(learn_more_url, content::Referrer(),
                             WindowOpenDisposition::NEW_FOREGROUND_TAB,
                             ui::PAGE_TRANSITION_LINK, false));
}

void DownloadProtectionService::SetDownloadPingToken(
    content::DownloadItem* item,
    const std::string& token) {
  if (item) {
    item->SetUserData(kDownloadPingTokenKey,
                      base::MakeUnique<DownloadPingToken>(token));
  }
}

std::string DownloadProtectionService::GetDownloadPingToken(
    const content::DownloadItem* item) {
  base::SupportsUserData::Data* token_data =
      item->GetUserData(kDownloadPingTokenKey);
  if (token_data)
    return static_cast<DownloadPingToken*>(token_data)->token_string();
  else
    return std::string();
}

namespace {
// Escapes a certificate attribute so that it can be used in a whitelist
// entry.  Currently, we only escape slashes, since they are used as a
// separator between attributes.
std::string EscapeCertAttribute(const std::string& attribute) {
  std::string escaped;
  for (size_t i = 0; i < attribute.size(); ++i) {
    if (attribute[i] == '%') {
      escaped.append("%25");
    } else if (attribute[i] == '/') {
      escaped.append("%2F");
    } else {
      escaped.push_back(attribute[i]);
    }
  }
  return escaped;
}
}  // namespace

// static
void DownloadProtectionService::GetCertificateWhitelistStrings(
    const net::X509Certificate& certificate,
    const net::X509Certificate& issuer,
    std::vector<std::string>* whitelist_strings) {
  // The whitelist paths are in the format:
  // cert/<ascii issuer fingerprint>[/CN=common_name][/O=org][/OU=unit]
  //
  // Any of CN, O, or OU may be omitted from the whitelist entry, in which
  // case they match anything.  However, the attributes that do appear will
  // always be in the order shown above.  At least one attribute will always
  // be present.

  const net::CertPrincipal& subject = certificate.subject();
  std::vector<std::string> ou_tokens;
  for (size_t i = 0; i < subject.organization_unit_names.size(); ++i) {
    ou_tokens.push_back(
        "/OU=" + EscapeCertAttribute(subject.organization_unit_names[i]));
  }

  std::vector<std::string> o_tokens;
  for (size_t i = 0; i < subject.organization_names.size(); ++i) {
    o_tokens.push_back("/O=" +
                       EscapeCertAttribute(subject.organization_names[i]));
  }

  std::string cn_token;
  if (!subject.common_name.empty()) {
    cn_token = "/CN=" + EscapeCertAttribute(subject.common_name);
  }

  std::set<std::string> paths_to_check;
  if (!cn_token.empty()) {
    paths_to_check.insert(cn_token);
  }
  for (size_t i = 0; i < o_tokens.size(); ++i) {
    paths_to_check.insert(cn_token + o_tokens[i]);
    paths_to_check.insert(o_tokens[i]);
    for (size_t j = 0; j < ou_tokens.size(); ++j) {
      paths_to_check.insert(cn_token + o_tokens[i] + ou_tokens[j]);
      paths_to_check.insert(o_tokens[i] + ou_tokens[j]);
    }
  }
  for (size_t i = 0; i < ou_tokens.size(); ++i) {
    paths_to_check.insert(cn_token + ou_tokens[i]);
    paths_to_check.insert(ou_tokens[i]);
  }

  std::string issuer_der;
  net::X509Certificate::GetDEREncoded(issuer.os_cert_handle(), &issuer_der);
  std::string hashed = base::SHA1HashString(issuer_der);
  std::string issuer_fp = base::HexEncode(hashed.data(), hashed.size());
  for (std::set<std::string>::iterator it = paths_to_check.begin();
       it != paths_to_check.end(); ++it) {
    whitelist_strings->push_back("cert/" + issuer_fp + *it);
  }
}

// static
GURL DownloadProtectionService::GetDownloadRequestUrl() {
  GURL url(kDownloadRequestUrl);
  std::string api_key = google_apis::GetAPIKey();
  if (!api_key.empty())
    url = url.Resolve("?key=" + net::EscapeQueryParamValue(api_key, true));

  return url;
}

std::unique_ptr<ReferrerChain> DownloadProtectionService::IdentifyReferrerChain(
    const content::DownloadItem& item) {
  // If navigation_observer_manager_ is null, return immediately. This could
  // happen in tests.
  if (!navigation_observer_manager_)
    return nullptr;

  std::unique_ptr<ReferrerChain> referrer_chain =
      base::MakeUnique<ReferrerChain>();
  content::WebContents* web_contents = item.GetWebContents();
  int download_tab_id = SessionTabHelper::IdForTab(web_contents);
  UMA_HISTOGRAM_BOOLEAN(
      "SafeBrowsing.ReferrerHasInvalidTabID.DownloadAttribution",
      download_tab_id == -1);
  // We look for the referrer chain that leads to the download url first.
  SafeBrowsingNavigationObserverManager::AttributionResult result =
      navigation_observer_manager_->IdentifyReferrerChainByEventURL(
          item.GetURL(), download_tab_id, kDownloadAttributionUserGestureLimit,
          referrer_chain.get());

  // If no navigation event is found, this download is not triggered by regular
  // navigation (e.g. html5 file apis, etc). We look for the referrer chain
  // based on relevant WebContents instead.
  if (result ==
          SafeBrowsingNavigationObserverManager::NAVIGATION_EVENT_NOT_FOUND &&
      web_contents && web_contents->GetLastCommittedURL().is_valid()) {
    AddEventUrlToReferrerChain(item, referrer_chain.get());
    result = navigation_observer_manager_->IdentifyReferrerChainByWebContents(
        web_contents, kDownloadAttributionUserGestureLimit,
        referrer_chain.get());
  }

  UMA_HISTOGRAM_COUNTS_100(
      "SafeBrowsing.ReferrerURLChainSize.DownloadAttribution",
      referrer_chain->size());
  UMA_HISTOGRAM_ENUMERATION(
      "SafeBrowsing.ReferrerAttributionResult.DownloadAttribution", result,
      SafeBrowsingNavigationObserverManager::ATTRIBUTION_FAILURE_TYPE_MAX);
  return referrer_chain;
}

void DownloadProtectionService::AddReferrerChainToPPAPIClientDownloadRequest(
    const GURL& initiating_frame_url,
    const GURL& initiating_main_frame_url,
    int tab_id,
    bool has_user_gesture,
    ClientDownloadRequest* out_request) {
  if (!navigation_observer_manager_)
    return;

  UMA_HISTOGRAM_BOOLEAN(
      "SafeBrowsing.ReferrerHasInvalidTabID.DownloadAttribution", tab_id == -1);
  SafeBrowsingNavigationObserverManager::AttributionResult result =
      navigation_observer_manager_->IdentifyReferrerChainByHostingPage(
          initiating_frame_url, initiating_main_frame_url, tab_id,
          has_user_gesture, kDownloadAttributionUserGestureLimit,
          out_request->mutable_referrer_chain());
  UMA_HISTOGRAM_COUNTS_100(
      "SafeBrowsing.ReferrerURLChainSize.PPAPIDownloadAttribution",
      out_request->referrer_chain_size());
  UMA_HISTOGRAM_ENUMERATION(
      "SafeBrowsing.ReferrerAttributionResult.PPAPIDownloadAttribution", result,
      SafeBrowsingNavigationObserverManager::ATTRIBUTION_FAILURE_TYPE_MAX);
}

}  // namespace safe_browsing

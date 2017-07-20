#include "chrome/browser/safe_browsing/check_client_download_request.h"

// TODO: this file inlclude download_protection_service and
// download_protection_service includes this file.
#include "chrome/browser/safe_browsing/download_protection_service.h"

// take these out of download_protection_service.cc
#include "base/rand_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/safe_browsing/file_type_policies.h"

// just copying them all for now...
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
#include "base/rand_util.h"
#include "base/scoped_observer.h"
#include "base/sequenced_task_runner_helpers.h"
#include "base/sha1.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/safe_browsing/download_feedback_service.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "chrome/browser/safe_browsing/sandboxed_zip_analyzer.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/safe_browsing/archive_analyzer_results.h"
#include "chrome/common/safe_browsing/binary_feature_extractor.h"
#include "chrome/common/safe_browsing/download_protection_util.h"
#include "chrome/common/safe_browsing/file_type_policies.h"
#include "chrome/common/url_constants.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/google/core/browser/google_util.h"
#include "components/history/core/browser/history_service.h"
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
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_status.h"

namespace safe_browsing {

// random stuff from .cc file
namespace {
/*
const int64_t kDownloadRequestTimeoutMs = 7000;
// We sample 1% of whitelisted downloads to still send out download pings.
const double kWhitelistDownloadSampleRate = 0.01;

// The number of user gestures we trace back for download attribution.
const int kDownloadAttributionUserGestureLimit = 2;
*/
const char kDownloadExtensionUmaName[] = "SBClientDownload.DownloadExtensions";
const char kUnsupportedSchemeUmaPrefix[] = "SBClientDownload.UnsupportedScheme";

const void* const kDownloadReferrerChainDataKey =
    &kDownloadReferrerChainDataKey;

enum WhitelistType {
  NO_WHITELIST_MATCH,
  URL_WHITELIST,
  SIGNATURE_WHITELIST,
  WHITELIST_TYPE_MAX
};

/*
void RecordCountOfWhitelistedDownload(WhitelistType type) {
  UMA_HISTOGRAM_ENUMERATION("SBClientDownload.CheckWhitelistResult", type,
                            WHITELIST_TYPE_MAX);
}
*/

void RecordFileExtensionType(const std::string& metric_name,
                             const base::FilePath& file) {
  UMA_HISTOGRAM_SPARSE_SLOWLY(
      metric_name, FileTypePolicies::GetInstance()->UmaValueForFile(file));
}
/*
void RecordArchivedArchiveFileExtensionType(const base::FilePath& file) {
  UMA_HISTOGRAM_SPARSE_SLOWLY(
      "SBClientDownload.ArchivedArchiveExtensions",
      FileTypePolicies::GetInstance()->UmaValueForFile(file));
}
*/
std::string GetUnsupportedSchemeName(const GURL& download_url) {
  if (download_url.SchemeIs(url::kContentScheme))
    return "ContentScheme";
  if (download_url.SchemeIs(url::kContentIDScheme))
    return "ContentIdScheme";
  if (download_url.SchemeIsFile())
    return download_url.has_host() ? "RemoteFileScheme" : "LocalFileScheme";
  if (download_url.SchemeIsFileSystem())
    return "FileSystemScheme";
  if (download_url.SchemeIs(url::kFtpScheme))
    return "FtpScheme";
  if (download_url.SchemeIs(url::kGopherScheme))
    return "GopherScheme";
  if (download_url.SchemeIs(url::kJavaScriptScheme))
    return "JavaScriptScheme";
  if (download_url.SchemeIsWSOrWSS())
    return "WSOrWSSScheme";
  return "OtherUnsupportedScheme";
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
/*
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
*/
}  // namespace

const char DownloadProtectionService::kDownloadRequestUrl[] =
    "https://sb-ssl.google.com/safebrowsing/clientreport/download";

const void* const DownloadProtectionService::kDownloadPingTokenKey =
    &kDownloadPingTokenKey;

// end random stuff from .cc file

CheckClientDownloadRequest::CheckClientDownloadRequest(
    content::DownloadItem* item,
    const CheckDownloadCallback& callback,
    DownloadProtectionService* service,
    const scoped_refptr<SafeBrowsingDatabaseManager>& database_manager,
    BinaryFeatureExtractor* binary_feature_extractor)
    : item_(item),
      url_chain_(item->GetUrlChain()),
      referrer_url_(item->GetReferrerUrl()),
      tab_url_(item->GetTabUrl()),
      tab_referrer_url_(item->GetTabReferrerUrl()),
      archived_executable_(false),
      archive_is_valid_(ArchiveValid::UNSET),
#if defined(OS_MACOSX)
      disk_image_signature_(nullptr),
#endif
      callback_(callback),
      service_(service),
      binary_feature_extractor_(binary_feature_extractor),
      database_manager_(database_manager),
      pingback_enabled_(service_->enabled()),
      finished_(false),
      type_(ClientDownloadRequest::WIN_EXECUTABLE),
      start_time_(base::TimeTicks::Now()),
      skipped_url_whitelist_(false),
      skipped_certificate_whitelist_(false),
      is_extended_reporting_(false),
      is_incognito_(false),
      weakptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  item_->AddObserver(this);
}

bool CheckClientDownloadRequest::ShouldSampleUnsupportedFile(
    const base::FilePath& filename) {
  // If this extension is specifically marked as SAMPLED_PING (as are
  // all "unknown" extensions), we may want to sample it. Sampling it means
  // we'll send a "light ping" with private info removed, and we won't
  // use the verdict.
  const FileTypePolicies* policies = FileTypePolicies::GetInstance();
  return service_ && is_extended_reporting_ && !is_incognito_ &&
         base::RandDouble() < policies->SampledPingProbability() &&
         policies->PingSettingForFile(filename) ==
             DownloadFileType::SAMPLED_PING;
}

void CheckClientDownloadRequest::Start() {
  DVLOG(2) << "Starting SafeBrowsing download check for: "
           << item_->DebugString(true);
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (item_->GetBrowserContext()) {
    Profile* profile = Profile::FromBrowserContext(item_->GetBrowserContext());
    is_extended_reporting_ =
        profile && IsExtendedReportingEnabled(*profile->GetPrefs());
    is_incognito_ = item_->GetBrowserContext()->IsOffTheRecord();
  }

  DownloadCheckResultReason reason = REASON_MAX;
  if (!IsSupportedDownload(*item_, item_->GetTargetFilePath(), &reason,
                           &type_)) {
    switch (reason) {
      case REASON_EMPTY_URL_CHAIN:
      case REASON_INVALID_URL:
      case REASON_LOCAL_FILE:
      case REASON_REMOTE_FILE:
        PostFinishTask(UNKNOWN, reason);
        return;
      case REASON_UNSUPPORTED_URL_SCHEME:
        RecordFileExtensionType(
            base::StringPrintf(
                "%s.%s", kUnsupportedSchemeUmaPrefix,
                GetUnsupportedSchemeName(item_->GetUrlChain().back()).c_str()),
            item_->GetTargetFilePath());
        PostFinishTask(UNKNOWN, reason);
        return;
      case REASON_NOT_BINARY_FILE:
        if (ShouldSampleUnsupportedFile(item_->GetTargetFilePath())) {
          // Send a "light ping" and don't use the verdict.
          type_ = ClientDownloadRequest::SAMPLED_UNSUPPORTED_FILE;
          break;
        }
        RecordFileExtensionType(kDownloadExtensionUmaName,
                                item_->GetTargetFilePath());
        PostFinishTask(UNKNOWN, reason);
        return;

      default:
        // We only expect the reasons explicitly handled above.
        NOTREACHED();
    }
  }
  RecordFileExtensionType(kDownloadExtensionUmaName,
                          item_->GetTargetFilePath());

  // Compute features from the file contents. Note that we record histograms
  // based on the result, so this runs regardless of whether the pingbacks
  // are enabled.
  if (item_->GetTargetFilePath().MatchesExtension(FILE_PATH_LITERAL(".zip"))) {
    StartExtractZipFeatures();
#if defined(OS_MACOSX)
  } else if (item_->GetTargetFilePath().MatchesExtension(
                 FILE_PATH_LITERAL(".dmg")) ||
             item_->GetTargetFilePath().MatchesExtension(
                 FILE_PATH_LITERAL(".img")) ||
             item_->GetTargetFilePath().MatchesExtension(
                 FILE_PATH_LITERAL(".iso")) ||
             item_->GetTargetFilePath().MatchesExtension(
                 FILE_PATH_LITERAL(".smi")) ||
             item_->GetTargetFilePath().MatchesExtension(
                 FILE_PATH_LITERAL(".cdr")) ||
             item_->GetTargetFilePath().MatchesExtension(
                 FILE_PATH_LITERAL(".dart")) ||
             item_->GetTargetFilePath().MatchesExtension(
                 FILE_PATH_LITERAL(".dc42")) ||
             item_->GetTargetFilePath().MatchesExtension(
                 FILE_PATH_LITERAL(".diskcopy42")) ||
             item_->GetTargetFilePath().MatchesExtension(
                 FILE_PATH_LITERAL(".dmgpart")) ||
             item_->GetTargetFilePath().MatchesExtension(
                 FILE_PATH_LITERAL(".dvdr")) ||
             item_->GetTargetFilePath().MatchesExtension(
                 FILE_PATH_LITERAL(".imgpart")) ||
             item_->GetTargetFilePath().MatchesExtension(
                 FILE_PATH_LITERAL(".ndif")) ||
             item_->GetTargetFilePath().MatchesExtension(
                 FILE_PATH_LITERAL(".sparsebundle")) ||
             item_->GetTargetFilePath().MatchesExtension(
                 FILE_PATH_LITERAL(".sparseimage")) ||
             item_->GetTargetFilePath().MatchesExtension(
                 FILE_PATH_LITERAL(".toast")) ||
             item_->GetTargetFilePath().MatchesExtension(
                 FILE_PATH_LITERAL(".udif"))) {
    StartExtractDmgFeatures();
#endif
  } else {
#if defined(OS_MACOSX)
    // Checks for existence of "koly" signature even if file doesn't have
    // archive-type extension, then calls ExtractFileOrDmgFeatures() with
    // result.
    BrowserThread::PostTaskAndReplyWithResult(
        BrowserThread::FILE, FROM_HERE,
        base::Bind(DiskImageTypeSnifferMac::IsAppleDiskImage,
                   item_->GetTargetFilePath()),
        base::Bind(&CheckClientDownloadRequest::ExtractFileOrDmgFeatures,
                   this));
#else
    StartExtractFileFeatures();
#endif
  }
}

// Start a timeout to cancel the request if it takes too long.
// This should only be called after we have finished accessing the file.
void CheckClientDownloadRequest::StartTimeout() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!service_) {
    // Request has already been cancelled.
    return;
  }
  timeout_start_time_ = base::TimeTicks::Now();
  BrowserThread::PostDelayedTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&CheckClientDownloadRequest::Cancel,
                     weakptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromMilliseconds(
          service_->download_request_timeout_ms()));
}

// Canceling a request will cause us to always report the result as UNKNOWN
// unless a pending request is about to call FinishRequest.
void CheckClientDownloadRequest::Cancel() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (fetcher_.get()) {
    // The DownloadProtectionService is going to release its reference, so we
    // might be destroyed before the URLFetcher completes.  Cancel the
    // fetcher so it does not try to invoke OnURLFetchComplete.
    fetcher_.reset();
  }
  // Note: If there is no fetcher, then some callback is still holding a
  // reference to this object.  We'll eventually wind up in some method on
  // the UI thread that will call FinishRequest() again.  If FinishRequest()
  // is called a second time, it will be a no-op.
  FinishRequest(UNKNOWN, REASON_REQUEST_CANCELED);
  // Calling FinishRequest might delete this object, we may be deleted by
  // this point.
}

// content::DownloadItem::Observer implementation.
void CheckClientDownloadRequest::OnDownloadDestroyed(
    content::DownloadItem* download) {
  Cancel();
  DCHECK(item_ == NULL);
}




// TODO: this method puts "DownloadProtectionService::" in front of a lot of stuff to avoid referencing the enums i copied to this .h file.
// From the net::URLFetcherDelegate interface.
void CheckClientDownloadRequest::OnURLFetchComplete(
    const net::URLFetcher* source) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_EQ(source, fetcher_.get());
  DVLOG(2) << "Received a response for URL: " << item_->GetUrlChain().back()
           << ": success=" << source->GetStatus().is_success()
           << " response_code=" << source->GetResponseCode();
  if (source->GetStatus().is_success()) {
    UMA_HISTOGRAM_SPARSE_SLOWLY("SBClientDownload.DownloadRequestResponseCode",
                                source->GetResponseCode());
  }
  UMA_HISTOGRAM_SPARSE_SLOWLY("SBClientDownload.DownloadRequestNetError",
                              -source->GetStatus().error());
  DownloadCheckResultReason reason = REASON_SERVER_PING_FAILED;
  DownloadProtectionService::DownloadCheckResult result = DownloadProtectionService::UNKNOWN;
  std::string token;
  if (source->GetStatus().is_success() &&
      net::HTTP_OK == source->GetResponseCode()) {
    ClientDownloadResponse response;
    std::string data;
    bool got_data = source->GetResponseAsString(&data);
    DCHECK(got_data);
    if (!response.ParseFromString(data)) {
      reason = REASON_INVALID_RESPONSE_PROTO;
      result = DownloadProtectionService::UNKNOWN;
    } else if (type_ == ClientDownloadRequest::SAMPLED_UNSUPPORTED_FILE) {
      // Ignore the verdict because we were just reporting a sampled file.
      reason = REASON_SAMPLED_UNSUPPORTED_FILE;
      result = DownloadProtectionService::UNKNOWN;
    } else {
      switch (response.verdict()) {
        case ClientDownloadResponse::SAFE:
          reason = REASON_DOWNLOAD_SAFE;
          result = DownloadProtectionService::SAFE;
          break;
        case ClientDownloadResponse::DANGEROUS:
          reason = REASON_DOWNLOAD_DANGEROUS;
          result = DownloadProtectionService::DANGEROUS;
          token = response.token();
          break;
        case ClientDownloadResponse::UNCOMMON:
          reason = REASON_DOWNLOAD_UNCOMMON;
          result = DownloadProtectionService::UNCOMMON;
          token = response.token();
          break;
        case ClientDownloadResponse::DANGEROUS_HOST:
          reason = REASON_DOWNLOAD_DANGEROUS_HOST;
          result = DownloadProtectionService::DANGEROUS_HOST;
          token = response.token();
          break;
        case ClientDownloadResponse::POTENTIALLY_UNWANTED:
          reason = REASON_DOWNLOAD_POTENTIALLY_UNWANTED;
          result = DownloadProtectionService::POTENTIALLY_UNWANTED;
          token = response.token();
          break;
        case ClientDownloadResponse::UNKNOWN:
          reason = REASON_VERDICT_UNKNOWN;
          result = DownloadProtectionService::UNKNOWN;
          break;
        default:
          LOG(DFATAL) << "Unknown download response verdict: "
                      << response.verdict();
          reason = REASON_INVALID_RESPONSE_VERDICT;
          result = DownloadProtectionService::UNKNOWN;
      }
    }

    if (!token.empty())
      DownloadProtectionService::SetDownloadPingToken(item_, token);

    bool upload_requested = response.upload();
    DownloadFeedbackService::MaybeStorePingsForDownload(
        result, upload_requested, item_, client_download_request_data_, data);
  }
  // We don't need the fetcher anymore.
  fetcher_.reset();
  UMA_HISTOGRAM_TIMES("SBClientDownload.DownloadRequestDuration",
                      base::TimeTicks::Now() - start_time_);
  UMA_HISTOGRAM_TIMES("SBClientDownload.DownloadRequestNetworkDuration",
                      base::TimeTicks::Now() - request_start_time_);

  FinishRequest(result, reason);
}

}  // namespace safe_browsing

#include "chrome/browser/safe_browsing/check_client_download_request.h"

// TODO: this file inlclude download_protection_service and download_protection_service includes this file. 
#include "chrome/browser/safe_browsing/download_protection_service.h"

// take these out of download_protection_service.cc
#include "chrome/common/safe_browsing/file_type_policies.h"
#include "base/rand_util.h"

namespace safe_browsing {

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

  bool CheckClientDownloadRequest::ShouldSampleUnsupportedFile(const base::FilePath& filename) {
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

}  // namespace safe_browsing

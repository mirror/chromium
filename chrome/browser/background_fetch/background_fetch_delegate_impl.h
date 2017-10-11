// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_IMPL_H_
#define CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_IMPL_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/background_fetch/background_fetch_offline_content_provider.h"
#include "components/download/public/download_params.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/background_fetch_delegate.h"
#include "url/origin.h"

class Profile;

namespace download {
class DownloadService;
}  // namespace download

// Implementation of BackgroundFetchDelegate using the DownloadService.
class BackgroundFetchDelegateImpl : public content::BackgroundFetchDelegate,
                                    public KeyedService {
 public:
  explicit BackgroundFetchDelegateImpl(Profile* profile);

  ~BackgroundFetchDelegateImpl() override;

  // KeyedService implementation:
  void Shutdown() override;

  // BackgroundFetchDelegate implementation:
  void CreateDownloadJob(
      const std::string& jobId,
      const std::string& title,
      const url::Origin& origin,
      int completed_parts,
      int total_parts,
      const std::vector<std::string>& current_guids) override;
  void DownloadUrl(const std::string& jobId,
                   const std::string& guid,
                   const std::string& method,
                   const GURL& url,
                   const net::NetworkTrafficAnnotationTag& traffic_annotation,
                   const net::HttpRequestHeaders& headers) override;

  void OnDownloadStarted(const std::string& guid,
                         std::unique_ptr<content::BackgroundFetchResponse>);

  void OnDownloadUpdated(const std::string& guid, uint64_t bytes_downloaded);

  void OnDownloadFailed(const std::string& guid,
                        download::Client::FailureReason reason);

  void OnDownloadSucceeded(const std::string& guid,
                           const base::FilePath& path,
                           uint64_t size);

  base::WeakPtr<BackgroundFetchDelegateImpl> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  struct JobDetails {
    JobDetails(JobDetails&&) = default;
    JobDetails(const url::Origin& origin, int completed_parts, int total_parts);
    ~JobDetails();

    std::string title;
    const url::Origin origin;
    int completed_parts;
    const int total_parts;

   private:
    DISALLOW_COPY_AND_ASSIGN(JobDetails);
  };

  void OnDownloadReceived(const std::string& guid,
                          download::DownloadParams::StartResult result);

  // The BackgroundFetchDelegateImplFactory depends on the
  // DownloadServiceFactory, so |download_service_| should outlive |this|.
  download::DownloadService* download_service_;

  // OfflineContentProvider that will show notifications for background fetches.
  BackgroundFetchOfflineContentProvider offline_content_provider_;

  // Map from individual download GUIDs to job ids.
  std::map<std::string, std::string> file_download_job_map_;

  // Map from job ids to the details of the job.
  std::map<std::string, JobDetails> job_details_map_;

  base::WeakPtrFactory<BackgroundFetchDelegateImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchDelegateImpl);
};

#endif  // CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_IMPL_H_

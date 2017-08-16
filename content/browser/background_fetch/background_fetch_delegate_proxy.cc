// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/background_fetch_delegate_proxy.h"

#include <unordered_map>
#include <utility>
#include <vector>

#include "base/debug/stack_trace.h"
#include "base/guid.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "content/browser/background_fetch/background_fetch_job_controller.h"
#include "content/public/browser/background_fetch_delegate.h"
#include "content/public/browser/background_fetch_response.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_url_parameters.h"

namespace content {

// Internal functionality of the BackgroundFetchDelegateProxy that lives on the
// UI thread, where all interaction with the download manager must happen.
class BackgroundFetchDelegateProxy::Core
    : public BackgroundFetchDelegate::Client {
 public:
  Core(const base::WeakPtr<BackgroundFetchDelegateProxy>& io_parent,
       BrowserContext* browser_context)
      : io_parent_(io_parent),
        browser_context_(browser_context),
        weak_ptr_factory_(this) {
    // Although the Core lives only on the UI thread, it is constructed on IO.
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    DCHECK(delegate_);

    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(&Core::InitializeDelegate,
                                           weak_ptr_factory_.GetWeakPtr()));
  }

  void InitializeDelegate() {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    delegate_ = browser_context_->GetBackgroundFetchDelegate();
    DCHECK(delegate_);
    delegate_->SetDelegateClient(weak_ptr_factory_.GetWeakPtr());
  }

  virtual ~Core() { DCHECK_CURRENTLY_ON(BrowserThread::UI); }

  base::WeakPtr<Core> GetWeakPtrOnUI() {
    return weak_ptr_factory_.GetWeakPtr();
  }

  void StartRequest(
      std::string guid,
      const base::WeakPtr<BackgroundFetchJobController>& job_controller,
      const url::Origin& origin,
      scoped_refptr<BackgroundFetchRequestInfo> request) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    DCHECK(request);
    DCHECK(delegate_ != nullptr);

    download_map_[guid] = request;

    const ServiceWorkerFetchRequest& fetch_request = request->fetch_request();

    const net::NetworkTrafficAnnotationTag traffic_annotation(
        net::DefineNetworkTrafficAnnotation("background_fetch_context",
                                            R"(
            semantics {
              sender: "Background Fetch API"
              description:
                "The Background Fetch API enables developers to upload or "
                "download files on behalf of the user. Such fetches will yield "
                "a user visible notification to inform the user of the "
                "operation, through which it can be suspended, resumed and/or "
                "cancelled. The developer retains control of the file once the "
                "fetch is completed,  similar to XMLHttpRequest and other "
                "mechanisms for fetching resources using JavaScript."
              trigger:
                "When the website uses the Background Fetch API to request "
                "fetching a file and/or a list of files. This is a Web "
                "Platform API for which no express user permission is required."
              data:
                "The request headers and data as set by the website's "
                "developer."
              destination: WEBSITE
            }
            policy {
              cookies_allowed: YES
              cookies_store: "user"
              setting: "This feature cannot be disabled in settings."
              policy_exception_justification: "Not implemented."
            })"));

    std::unique_ptr<DownloadUrlParameters> download_parameters(
        base::MakeUnique<DownloadUrlParameters>(fetch_request.url, nullptr,
                                                traffic_annotation));

    // TODO(peter): The |download_parameters| should be populated with all the
    // properties set in the |fetch_request| structure.

    for (const auto& pair : fetch_request.headers)
      download_parameters->add_request_header(pair.first, pair.second);

    // Append the Origin header for requests whose CORS flag is set, or whose
    // request method is not GET or HEAD. See section 3.1 of the standard:
    // https://fetch.spec.whatwg.org/#origin-header
    if (fetch_request.mode == FETCH_REQUEST_MODE_CORS ||
        fetch_request.mode == FETCH_REQUEST_MODE_CORS_WITH_FORCED_PREFLIGHT ||
        (fetch_request.method != "GET" && fetch_request.method != "POST")) {
      download_parameters->add_request_header("Origin", origin.Serialize());
    }

    download_parameters->set_transient(true);

    delegate_->DownloadUrl(guid, std::move(download_parameters), request);
  }

 private:
  void OnDownloadStarted(
      const std::string& guid,
      std::unique_ptr<const BackgroundFetchResponse> response) override;

  void OnDownloadUpdated(const std::string& guid,
                         uint64_t bytes_downloaded) override;

  void OnDownloadFailed(const std::string& guid,
                        BackgroundFetchDelegate::FailureReason reason) override;

  void OnDownloadSucceeded(const std::string& guid,
                           const base::FilePath& path,
                           uint64_t size) override;

  void OnDownloadReceived(const std::string& guid,
                          BackgroundFetchDelegate::StartResult result) override;

  // Weak reference to the BackgroundFetchJobController instance that owns us.
  base::WeakPtr<BackgroundFetchDelegateProxy> io_parent_;

  // The BrowserContext that owns the JobController, and thereby us.
  BrowserContext* browser_context_;

  BackgroundFetchDelegate* delegate_;

  // Map from DownloadService GUIDs to the RequestInfo.
  std::unordered_map<std::string, scoped_refptr<BackgroundFetchRequestInfo>>
      download_map_;

  base::WeakPtrFactory<Core> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(Core);
};

void BackgroundFetchDelegateProxy::Core::OnDownloadStarted(
    const std::string& guid,
    std::unique_ptr<const BackgroundFetchResponse> response) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto request = download_map_[guid];
  DCHECK(request.get() != nullptr);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&BackgroundFetchDelegateProxy::DidStartRequest, io_parent_,
                     guid, std::move(response)));
}

void BackgroundFetchDelegateProxy::Core::OnDownloadUpdated(
    const std::string& guid,
    uint64_t bytes_downloaded) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // TODO(delphick): Do something on updates?
}

void BackgroundFetchDelegateProxy::Core::OnDownloadFailed(
    const std::string& guid,
    BackgroundFetchDelegate::FailureReason reason) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto request = download_map_[guid];
  DCHECK(request.get() != nullptr);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&BackgroundFetchDelegateProxy::OnDownloadFailed,
                     io_parent_, guid, reason));
}

void BackgroundFetchDelegateProxy::Core::OnDownloadSucceeded(
    const std::string& guid,
    const base::FilePath& path,
    uint64_t size) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&BackgroundFetchDelegateProxy::OnDownloadSucceeded,
                     io_parent_, guid, path, size));
}

void BackgroundFetchDelegateProxy::Core::OnDownloadReceived(
    const std::string& guid,
    BackgroundFetchDelegate::StartResult result) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&BackgroundFetchDelegateProxy::OnDownloadReceived,
                     io_parent_, guid, result));
}

BackgroundFetchDelegateProxy::BackgroundFetchDelegateProxy(
    BrowserContext* browser_context)
    : weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  ui_core_.reset(new Core(weak_ptr_factory_.GetWeakPtr(), browser_context));

  // Get a WeakPtr over which we can talk to the |ui_core_|. Normally it
  // would be unsafe to obtain a weak pointer on the IO thread from a
  // factory that lives on the UI thread, but it's ok in this constructor
  // since the Core can't be destroyed before this constructor finishes.
  ui_core_ptr_ = ui_core_->GetWeakPtrOnUI();
}

BackgroundFetchDelegateProxy::~BackgroundFetchDelegateProxy() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

void BackgroundFetchDelegateProxy::OnDownloadSucceeded(std::string guid,
                                                       base::FilePath path,
                                                       uint64_t size) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  scoped_refptr<BackgroundFetchRequestInfo> request_info;
  base::WeakPtr<BackgroundFetchJobController> job_controller;
  std::tie(request_info, job_controller) = controller_map_[guid];

  request_info->SetPathAndSize(path, size);
  request_info->SetDownloadStatePopulated();

  if (job_controller) {
    job_controller->DidCompleteRequest(request_info);
  }
}

void BackgroundFetchDelegateProxy::OnDownloadFailed(
    std::string guid,
    BackgroundFetchDelegate::FailureReason reason) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  scoped_refptr<BackgroundFetchRequestInfo> request_info;
  base::WeakPtr<BackgroundFetchJobController> job_controller;
  std::tie(request_info, job_controller) = controller_map_[guid];

  if (job_controller) {
    job_controller->DidCompleteRequest(request_info);
  }
}

void BackgroundFetchDelegateProxy::OnDownloadReceived(
    const std::string& guid,
    BackgroundFetchDelegate::StartResult result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // TODO(delphick): do something here.
}

void BackgroundFetchDelegateProxy::StartRequest(
    BackgroundFetchJobController* job_controller,
    scoped_refptr<BackgroundFetchRequestInfo> request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // TODO(delphick): do we have to generate a GUID? docs say it will generate
  // one. DCHECKs also force it to be upper case.
  std::string guid(base::ToUpperASCII(base::GenerateGUID()));

  controller_map_[guid] = std::make_pair(request, job_controller->GetWeakPtr());

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&Core::StartRequest, ui_core_ptr_, guid,
                     job_controller->GetWeakPtr(),
                     job_controller->registration_id().origin(), request));
}

void BackgroundFetchDelegateProxy::UpdateUI(const std::string& title) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // TODO(delphick): Update the user interface with |title|.
}

void BackgroundFetchDelegateProxy::Abort() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // TODO(delphick): Abort all in-progress downloads.
}

void BackgroundFetchDelegateProxy::DidStartRequest(
    const std::string& guid,
    std::unique_ptr<const BackgroundFetchResponse> response) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  base::WeakPtr<BackgroundFetchJobController> job_controller;
  scoped_refptr<BackgroundFetchRequestInfo> request;

  std::tie(request, job_controller) = controller_map_[guid];

  request->PopulateWithResponse(std::move(response));

  if (job_controller) {
    job_controller->DidStartRequest(request, guid);
  }
}

}  // namespace content

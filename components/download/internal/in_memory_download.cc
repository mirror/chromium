// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/in_memory_download.h"

#include <memory>

#include "base/bind.h"
#include "base/strings/string_util.h"
#include "components/download/internal/blob_task_proxy.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_storage_context.h"

namespace download {

namespace {

// Converts a string to HTTP method used by URLFetcher.
net::URLFetcher::RequestType ToRequestType(const std::string& method) {
  // Only supports GET and POST.
  if (base::EqualsCaseInsensitiveASCII(method, "GET"))
    return net::URLFetcher::RequestType::GET;
  if (base::EqualsCaseInsensitiveASCII(method, "POST"))
    return net::URLFetcher::RequestType::POST;

  NOTREACHED();
  return net::URLFetcher::RequestType::GET;
}

}  // namespace

InMemoryDownload::InMemoryDownload(
    const std::string& guid,
    const RequestParams& request_params,
    const net::NetworkTrafficAnnotationTag& traffic_annotation,
    Delegate* delegate,
    scoped_refptr<net::URLRequestContextGetter> request_context_getter,
    BlobTaskProxy::BlobContextGetter blob_context_getter,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
    : guid_(guid),
      request_params_(request_params),
      traffic_annotation_(traffic_annotation),
      request_context_getter_(request_context_getter),
      blob_task_proxy_(BlobTaskProxy::Create(std::move(blob_context_getter),
                                             io_task_runner)),
      io_task_runner_(io_task_runner),
      delegate_(delegate),
      weak_ptr_factory_(this) {
  DCHECK(!guid_.empty());
}

InMemoryDownload::~InMemoryDownload() {
  // Say goodbye to IO thread proxy.
  io_task_runner_->DeleteSoon(FROM_HERE, blob_task_proxy_.release());
}

void InMemoryDownload::Start() {
  url_fetcher_ = net::URLFetcher::Create(request_params_.url,
                                         ToRequestType(request_params_.method),
                                         this, traffic_annotation_);
  url_fetcher_->SetRequestContext(request_context_getter_.get());
  url_fetcher_->SetExtraRequestHeaders(
      request_params_.request_headers.ToString());
  url_fetcher_->Start();
  state_ = State::IN_PROGRESS;
}

void InMemoryDownload::OnURLFetchDownloadProgress(
    const net::URLFetcher* source,
    int64_t current,
    int64_t total,
    int64_t current_network_bytes) {
  bytes_downloaded_ = current;

  if (delegate_)
    delegate_->OnDownloadProgress(this);
}

void InMemoryDownload::OnURLFetchComplete(const net::URLFetcher* source) {
  switch (source->GetStatus().status()) {
    case net::URLRequestStatus::Status::SUCCESS:
      if (HandleResponseCode(source->GetResponseCode())) {
        SaveAsBlob();
        return;
      }

      state_ = State::FAILED;
      NotifyDelegateDownloadComplete();
      return;
    case net::URLRequestStatus::Status::IO_PENDING:
      return;
    case net::URLRequestStatus::Status::CANCELED:
    case net::URLRequestStatus::Status::FAILED:
      state_ = State::FAILED;
      NotifyDelegateDownloadComplete();
      break;
  }
}

bool InMemoryDownload::HandleResponseCode(int response_code) {
  switch (response_code) {
    case -1:  // Non-HTTP request.
    case net::HTTP_OK:
    case net::HTTP_NON_AUTHORITATIVE_INFORMATION:
    case net::HTTP_PARTIAL_CONTENT:
    case net::HTTP_CREATED:
    case net::HTTP_ACCEPTED:
    case net::HTTP_NO_CONTENT:
    case net::HTTP_RESET_CONTENT:
      return true;
    // All other codes are considered as failed.
    default:
      return false;
  }
  NOTREACHED();
  return false;
}

void InMemoryDownload::SaveAsBlob() {
  DCHECK(url_fetcher_);

  // This will copy the internal memory in |url_fetcher| into |data|.
  // TODO(xingliu): Use response writer to avoid two extra copies.
  std::unique_ptr<std::string> data;
  DCHECK(url_fetcher_->GetResponseAsString(data.get()));

  auto callback = base::BindOnce(&InMemoryDownload::OnSaveBlobDone,
                                 weak_ptr_factory_.GetWeakPtr());
  auto save_blob_task = base::BindOnce(&BlobTaskProxy::SaveAsBlobOnIO,
                                       base::Unretained(blob_task_proxy_.get()),
                                       std::move(data), std::move(callback));

  io_task_runner_->PostTask(FROM_HERE, std::move(save_blob_task));
}

void InMemoryDownload::OnSaveBlobDone(
    std::unique_ptr<storage::BlobDataHandle> blob_handle) {
  state_ = (blob_handle->GetBlobStatus() == storage::BlobStatus::DONE)
               ? State::COMPLETE
               : State::FAILED;
  blob_data_handle_ = std::move(blob_handle);

  NotifyDelegateDownloadComplete();
}

void InMemoryDownload::NotifyDelegateDownloadComplete() {
  if (delegate_)
    delegate_->OnDownloadComplete(this);
}

}  // namespace download

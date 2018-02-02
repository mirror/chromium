// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_use_measurement/content/service_url_loader.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"

namespace data_use_measurement {

// static
std::unique_ptr<content::SimpleURLLoader> ServiceURLLoader::Create(
    std::unique_ptr<network::ResourceRequest> resource_request,
    const net::NetworkTrafficAnnotationTag& annotation_tag,
    ServiceName service_name) {
  std::unique_ptr<content::SimpleURLLoader> default_loader =
      content::SimpleURLLoader::Create(std::move(resource_request),
                                       annotation_tag);

  return std::make_unique<ServiceURLLoader>(std::move(default_loader),
                                            service_name);
}

ServiceURLLoader::ServiceURLLoader(
    std::unique_ptr<content::SimpleURLLoader> default_loader,
    ServiceName service_name)
    : default_loader_(std::move(default_loader)), service_name_(service_name) {
  DCHECK(default_loader_);
  DCHECK_NE(service_name_, ServiceName::kCount);
  // TODO(ryansturm): Set a default redirect callback. Track background
  // foreground.
  UMA_HISTOGRAM_ENUMERATION("ServiceURLLoader.ServiceType", service_name_,
                            ServiceName::kCount);
}

ServiceURLLoader::~ServiceURLLoader() = default;

// content::SimpleURLLoader implementation.
void ServiceURLLoader::DownloadToString(
    network::mojom::URLLoaderFactory* url_loader_factory,
    content::SimpleURLLoader::BodyAsStringCallback body_as_string_callback,
    size_t max_body_size) {
  // TODO(ryansturm): Wrap the passed in callback.
  default_loader_->DownloadToString(
      url_loader_factory, std::move(body_as_string_callback), max_body_size);
}

void ServiceURLLoader::DownloadToStringOfUnboundedSizeUntilCrashAndDie(
    network::mojom::URLLoaderFactory* url_loader_factory,
    content::SimpleURLLoader::BodyAsStringCallback body_as_string_callback) {
  // TODO(ryansturm): Wrap the passed in callback.
  default_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory, std::move(body_as_string_callback));
}

void ServiceURLLoader::DownloadToFile(
    network::mojom::URLLoaderFactory* url_loader_factory,
    content::SimpleURLLoader::DownloadToFileCompleteCallback
        download_to_file_complete_callback,
    const base::FilePath& file_path,
    int64_t max_body_size) {
  // TODO(ryansturm): Wrap the passed in callback.
  default_loader_->DownloadToFile(url_loader_factory,
                                  std::move(download_to_file_complete_callback),
                                  file_path, max_body_size);
}

void ServiceURLLoader::DownloadToTempFile(
    network::mojom::URLLoaderFactory* url_loader_factory,
    content::SimpleURLLoader::DownloadToFileCompleteCallback
        download_to_file_complete_callback,
    int64_t max_body_size) {
  // TODO(ryansturm): Wrap the passed in callback.
  default_loader_->DownloadToTempFile(
      url_loader_factory, std::move(download_to_file_complete_callback),
      max_body_size);
}

void ServiceURLLoader::SetOnRedirectCallback(
    const content::SimpleURLLoader::OnRedirectCallback& on_redirect_callback) {
  // TODO(ryansturm): Wrap the passed in callback.
  default_loader_->SetOnRedirectCallback(on_redirect_callback);
}

void ServiceURLLoader::SetAllowPartialResults(bool allow_partial_results) {
  default_loader_->SetAllowPartialResults(allow_partial_results);
}

void ServiceURLLoader::SetAllowHttpErrorResults(bool allow_http_error_results) {
  default_loader_->SetAllowHttpErrorResults(allow_http_error_results);
}

void ServiceURLLoader::AttachStringForUpload(
    const std::string& upload_data,
    const std::string& upload_content_type) {
  default_loader_->AttachStringForUpload(upload_data, upload_content_type);
}

void ServiceURLLoader::AttachFileForUpload(
    const base::FilePath& upload_file_path,
    const std::string& upload_content_type) {
  default_loader_->AttachFileForUpload(upload_file_path, upload_content_type);
}

void ServiceURLLoader::SetRetryOptions(int max_retries, int retry_mode) {
  default_loader_->SetRetryOptions(max_retries, retry_mode);
}

int ServiceURLLoader::NetError() const {
  return default_loader_->NetError();
}

const network::ResourceResponseHead* ServiceURLLoader::ResponseInfo() const {
  return default_loader_->ResponseInfo();
}

}  // namespace data_use_measurement

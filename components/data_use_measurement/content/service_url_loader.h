// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_USE_MEASUREMENT_CONTENT_SERVICE_URL_LOADER_H_
#define COMPONENTS_DATA_USE_MEASUREMENT_CONTENT_SERVICE_URL_LOADER_H_

#include <memory>

#include "base/macros.h"
#include "content/public/common/simple_url_loader.h"

namespace data_use_measurement {

// This class should be used by all services so that their data use can be
// tracked to verify that no services are using more data than they should.
class ServiceURLLoader : public content::SimpleURLLoader {
 public:
  enum class ServiceName {
    // Other should ideally never be used. Please add a new type if needed.
    kOther = 0,
    // URL loads related to the data reduction proxy.
    kDataReductionProxy = 1,
    kCount = kDataReductionProxy + 1,
  };

  // Creates a default SimpleURLLoader (provided by
  // content::SimpleURLLoader::Create) wrapped in a ServiceURLLoader.
  static std::unique_ptr<content::SimpleURLLoader> Create(
      std::unique_ptr<network::ResourceRequest> resource_request,
      const net::NetworkTrafficAnnotationTag& annotation_tag,
      ServiceName service_name);

  // |default_loader| will load URL and all calls to ServiceURLLoader will be
  // forwarded to |default_loader|. |service_name| is the service that is
  // requesting the URL load.
  ServiceURLLoader(std::unique_ptr<content::SimpleURLLoader> default_loader,
                   ServiceName service_name);
  ~ServiceURLLoader() override;

  // content::SimpleURLLoader implementation.
  void DownloadToString(
      network::mojom::URLLoaderFactory* url_loader_factory,
      content::SimpleURLLoader::BodyAsStringCallback body_as_string_callback,
      size_t max_body_size) override;
  void DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      network::mojom::URLLoaderFactory* url_loader_factory,
      content::SimpleURLLoader::BodyAsStringCallback body_as_string_callback)
      override;
  void DownloadToFile(network::mojom::URLLoaderFactory* url_loader_factory,
                      content::SimpleURLLoader::DownloadToFileCompleteCallback
                          download_to_file_complete_callback,
                      const base::FilePath& file_path,
                      int64_t max_body_size) override;
  void DownloadToTempFile(
      network::mojom::URLLoaderFactory* url_loader_factory,
      content::SimpleURLLoader::DownloadToFileCompleteCallback
          download_to_file_complete_callback,
      int64_t max_body_size) override;
  void SetOnRedirectCallback(const content::SimpleURLLoader::OnRedirectCallback&
                                 on_redirect_callback) override;
  void SetAllowPartialResults(bool allow_partial_results) override;
  void SetAllowHttpErrorResults(bool allow_http_error_results) override;
  void AttachStringForUpload(const std::string& upload_data,
                             const std::string& upload_content_type) override;
  void AttachFileForUpload(const base::FilePath& upload_file_path,
                           const std::string& upload_content_type) override;
  void SetRetryOptions(int max_retries, int retry_mode) override;
  int NetError() const override;
  const network::ResourceResponseHead* ResponseInfo() const override;

 private:
  // A SimpleURLLoader that performs all loading actions.
  std::unique_ptr<content::SimpleURLLoader> default_loader_;

  // The service that is requesting the URL load.
  ServiceName service_name_;

  DISALLOW_COPY_AND_ASSIGN(ServiceURLLoader);
};

}  // namespace data_use_measurement

#endif  // COMPONENTS_DATA_USE_MEASUREMENT_CONTENT_SERVICE_URL_LOADER_H_

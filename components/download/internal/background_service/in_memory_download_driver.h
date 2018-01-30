// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_IN_MEMORY_DOWNLOAD_DRIVER_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_IN_MEMORY_DOWNLOAD_DRIVER_H_

#include "components/download/internal/background_service/download_driver.h"

#include <map>
#include <memory>

#include "base/macros.h"
#include "components/download/internal/background_service/in_memory_download.h"

namespace download {

class InMemoryDownload;

// Download backend that keep data in memory and doesn't persist to disk.
class InMemoryDownloadDriver : public DownloadDriver,
                               public InMemoryDownload::Delegate {
 public:
  InMemoryDownloadDriver(
      scoped_refptr<net::URLRequestContextGetter> request_context_getter,
      BlobTaskProxy::BlobContextGetter blob_context_getter,
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner);
  ~InMemoryDownloadDriver() override;

 private:
  // DownloadDriver implementation.
  void Initialize(DownloadDriver::Client* client) override;
  void HardRecover() override;
  bool IsReady() const override;
  void Start(
      const RequestParams& request_params,
      const std::string& guid,
      const base::FilePath& file_path,
      const net::NetworkTrafficAnnotationTag& traffic_annotation) override;
  void Remove(const std::string& guid) override;
  void Pause(const std::string& guid) override;
  void Resume(const std::string& guid) override;
  base::Optional<DriverEntry> Find(const std::string& guid) override;
  std::set<std::string> GetActiveDownloads() override;
  size_t EstimateMemoryUsage() const override;

  // InMemoryDownload::Delegate implementation.
  void OnDownloadProgress(InMemoryDownload* download) override;
  void OnDownloadComplete(InMemoryDownload* download) override;

  // The client that receives updates from low level download logic.
  DownloadDriver::Client* client_;

  // A map of GUID and in memory download, which holds download data.
  std::map<std::string, std::unique_ptr<InMemoryDownload>> downloads_;

  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  BlobTaskProxy::BlobContextGetter blob_context_getter_;
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(InMemoryDownloadDriver);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_IN_MEMORY_DOWNLOAD_DRIVER_H_

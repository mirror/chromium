// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_IN_MEMORY_DOWNLOAD_DRIVER_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_IN_MEMORY_DOWNLOAD_DRIVER_H_

#include "base/macros.h"
#include "components/download/internal/download_driver.h"

namespace download {

class InMemoryDownloadDriver : public DownloadDriver {
 public:
  InMemoryDownloadDriver();
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

  DISALLOW_COPY_AND_ASSIGN(InMemoryDownloadDriver);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_IN_MEMORY_DOWNLOAD_DRIVER_H_

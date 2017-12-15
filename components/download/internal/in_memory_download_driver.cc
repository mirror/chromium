// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/in_memory_download_driver.h"

namespace download {

InMemoryDownloadDriver::InMemoryDownloadDriver() {}

InMemoryDownloadDriver::~InMemoryDownloadDriver() {}

void InMemoryDownloadDriver::Initialize(DownloadDriver::Client* client) {}

void InMemoryDownloadDriver::HardRecover() {}

bool InMemoryDownloadDriver::IsReady() const {
  return true;
}

void InMemoryDownloadDriver::Start(
    const RequestParams& request_params,
    const std::string& guid,
    const base::FilePath& file_path,
    const net::NetworkTrafficAnnotationTag& traffic_annotation) {}

void InMemoryDownloadDriver::Remove(const std::string& guid) {}

void InMemoryDownloadDriver::Pause(const std::string& guid) {}

void InMemoryDownloadDriver::Resume(const std::string& guid) {}

base::Optional<DriverEntry> InMemoryDownloadDriver::Find(
    const std::string& guid) {
  return base::Optional<DriverEntry>();
}

std::set<std::string> InMemoryDownloadDriver::GetActiveDownloads() {
  std::set<std::string> downloads;
  return downloads;
}

}  // namespace download

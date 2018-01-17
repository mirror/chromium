// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/in_memory_download_driver.h"

namespace download {

namespace {

// Helper function to create download driver entry based on in memory download.
DriverEntry ToDriverEntry(const InMemoryDownload& download) {
  DriverEntry entry;
  // TODO(xingliu): Implement.

  return entry;
}

}  // namespace

InMemoryDownloadDriver::InMemoryDownloadDriver(
    scoped_refptr<net::URLRequestContextGetter> request_context_getter,
    BlobTaskProxy::BlobContextGetter blob_context_getter,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
    : request_context_getter_(request_context_getter),
      blob_context_getter_(blob_context_getter),
      io_task_runner_(io_task_runner) {}

InMemoryDownloadDriver::~InMemoryDownloadDriver() {}

void InMemoryDownloadDriver::Initialize(DownloadDriver::Client* client) {
  DCHECK(!client_) << "Initialize can be called only once.";
  client_ = client;
}

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
  base::Optional<DriverEntry> entry;
  auto it = downloads_.find(guid);
  if (it != downloads_.end())
    entry = ToDriverEntry(*it->second.get());
  return entry;
}

std::set<std::string> InMemoryDownloadDriver::GetActiveDownloads() {
  std::set<std::string> downloads;
  for (auto it = downloads_.begin(); it != downloads_.end(); ++it) {
    if (it->second->state() == InMemoryDownload::State::INITIAL ||
        it->second->state() == InMemoryDownload::State::IN_PROGRESS) {
      downloads.emplace(it->first);
    }
  }
  return downloads;
}

size_t InMemoryDownloadDriver::EstimateMemoryUsage() const {
  return 0u;
}

void InMemoryDownloadDriver::OnDownloadProgress(InMemoryDownload* download) {}

void InMemoryDownloadDriver::OnDownloadComplete(InMemoryDownload* download) {}

}  // namespace download

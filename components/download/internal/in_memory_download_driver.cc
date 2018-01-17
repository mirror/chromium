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
    const net::NetworkTrafficAnnotationTag& traffic_annotation) {
  std::unique_ptr<InMemoryDownload> download =
      std::make_unique<InMemoryDownload>(
          guid, request_params, traffic_annotation, this,
          request_context_getter_, blob_context_getter_, io_task_runner_);
  InMemoryDownload* download_ptr = download.get();
  downloads_.emplace(guid, std::move(download));
  download_ptr->Start();
}

void InMemoryDownloadDriver::Remove(const std::string& guid) {
  downloads_.erase(guid);
}

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
  for (const auto& it : downloads_) {
    if (it.second->state() == InMemoryDownload::State::INITIAL ||
        it.second->state() == InMemoryDownload::State::IN_PROGRESS) {
      downloads.emplace(it.first);
    }
  }
  return downloads;
}

size_t InMemoryDownloadDriver::EstimateMemoryUsage() const {
  size_t memory_usage = 0u;
  for (const auto& it : downloads_) {
    memory_usage += it.second->EstimateMemoryUsage();
  }
  return memory_usage;
}

void InMemoryDownloadDriver::OnDownloadProgress(InMemoryDownload* download) {}

void InMemoryDownloadDriver::OnDownloadComplete(InMemoryDownload* download) {}

}  // namespace download

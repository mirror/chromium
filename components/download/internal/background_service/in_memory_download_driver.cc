// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/background_service/in_memory_download_driver.h"

namespace download {

namespace {

DriverEntry::State ToDriverEntryState(InMemoryDownload::State state) {
  switch (state) {
    case InMemoryDownload::State::INITIAL:
      return DriverEntry::State::IN_PROGRESS;
    case InMemoryDownload::State::IN_PROGRESS:
      return DriverEntry::State::IN_PROGRESS;
    case InMemoryDownload::State::FAILED:
      return DriverEntry::State::INTERRUPTED;
    case InMemoryDownload::State::COMPLETE:
      return DriverEntry::State::COMPLETE;
  }
  NOTREACHED();
  return DriverEntry::State::UNKNOWN;
}

// Helper function to create download driver entry based on in memory download.
DriverEntry CreateDriverEntry(const InMemoryDownload& download) {
  DriverEntry entry;
  entry.guid = download.guid();
  entry.state = ToDriverEntryState(download.state());
  entry.paused = false;
  entry.done = entry.state == DriverEntry::State::INTERRUPTED ||
               entry.state == DriverEntry::State::COMPLETE ||
               entry.state == DriverEntry::State::CANCELLED;
  entry.bytes_downloaded = download.bytes_downloaded();
  entry.response_headers = download.response_headers();
  if (entry.response_headers) {
    entry.expected_total_size = entry.response_headers->GetContentLength();
  }
  // TODO(xingliu): See if we can support pause/resume, and can_resume.
  // UrlFetcher doesn't expose url chain. Figure out if empty url chain is fine.
  entry.can_resume = false;
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

InMemoryDownloadDriver::~InMemoryDownloadDriver() = default;

void InMemoryDownloadDriver::Initialize(DownloadDriver::Client* client) {
  DCHECK(!client_) << "Initialize can be called only once.";
  client_ = client;
  DCHECK(client_);
  client_->OnDriverReady(true);
}

void InMemoryDownloadDriver::HardRecover() {
  client_->OnDriverHardRecoverComplete(true);
}

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
  client_->OnDownloadCreated(CreateDriverEntry(*download_ptr));
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
    entry = CreateDriverEntry(*it->second.get());
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

void InMemoryDownloadDriver::OnDownloadProgress(InMemoryDownload* download) {
  DCHECK(client_);
  client_->OnDownloadUpdated(CreateDriverEntry(*download));
}

void InMemoryDownloadDriver::OnDownloadComplete(InMemoryDownload* download) {
  DCHECK(download);
  DCHECK(client_);
  DriverEntry entry = CreateDriverEntry(*download);
  switch (download->state()) {
    case InMemoryDownload::State::FAILED:
      // TODO(xingliu): Support recoverable.
      client_->OnDownloadFailed(entry, FailureType::NOT_RECOVERABLE);
      return;
    case InMemoryDownload::State::COMPLETE:
      client_->OnDownloadSucceeded(entry);
      return;
    case InMemoryDownload::State::INITIAL:
    case InMemoryDownload::State::IN_PROGRESS:
      NOTREACHED();
      return;
  }
}

}  // namespace download

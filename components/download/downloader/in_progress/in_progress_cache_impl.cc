// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/downloader/in_progress/in_progress_cache_impl.h"

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/task_runner_util.h"
#include "components/download/downloader/in_progress/in_progress_conversions.h"
#include "content/public/browser/browser_thread.h"

namespace download {

namespace {

// Helper functions for |entries_| related operations.
int GetIndexFromEntries(const metadata_pb::DownloadEntries& entries,
                        const std::string& guid) {
  int size_of_entries = entries.entries_size();
  for (int i = 0; i < size_of_entries; i++) {
    if (entries.entries(i).guid() == guid)
      return i;
  }
  return -1;
}

void AddOrReplaceEntryInEntries(metadata_pb::DownloadEntries& entries,
                                const DownloadEntry& entry) {
  metadata_pb::DownloadEntry metadata_entry =
      InProgressConversions::DownloadEntryToProto(entry);
  int entry_index = GetIndexFromEntries(entries, metadata_entry.guid());
  metadata_pb::DownloadEntry* entry_ptr =
      (entry_index < 0) ? entries.add_entries()
                        : entries.mutable_entries(entry_index);
  *entry_ptr = metadata_entry;
}

base::Optional<DownloadEntry> GetEntryFromEntries(
    const metadata_pb::DownloadEntries& entries,
    const std::string& guid) {
  int entry_index = GetIndexFromEntries(entries, guid);
  if (entry_index < 0)
    return base::nullopt;
  return InProgressConversions::DownloadEntryFromProto(
      entries.entries(entry_index));
}

void RemoveEntryFromEntries(metadata_pb::DownloadEntries& entries,
                            const std::string& guid) {
  int entry_index = GetIndexFromEntries(entries, guid);
  if (entry_index >= 0)
    entries.mutable_entries()->DeleteSubrange(entry_index, 1);
}

// Helper functions for file read/write operations.
std::unique_ptr<char[]> ReadEntriesFromFile(base::FilePath file_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  // Check validity of file.
  base::File entries_file(file_path,
                          base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!entries_file.IsValid()) {
    LOG(ERROR) << "Could not read download entries from file "
               << "because file is not valid.";
    return nullptr;
  }

  // Get file info.
  base::File::Info info;
  if (!entries_file.GetInfo(&info)) {
    LOG(ERROR) << "Could not read download entries from file "
               << "because get info failed.";
    return nullptr;
  }

  if (info.is_directory) {
    LOG(ERROR) << "Could not read download entries from file "
               << "because file is a directory.";
    return nullptr;
  }

  // Read and parse file.
  const int64_t size = base::saturated_cast<int>(info.size);
  DCHECK(size <= INT_MAX);
  auto file_data = std::make_unique<char[]>(info.size);
  if (!entries_file.Read(0, file_data.get(), size)) {
    LOG(ERROR) << "Could not read download entries from file "
               << "because there was a read failure.";
    return nullptr;
  }

  return file_data;
}

std::string EntriesToString(const metadata_pb::DownloadEntries& entries) {
  std::string entries_string;
  if (!entries.SerializeToString(&entries_string)) {
    // TODO(crbug.com/778425): Have more robust error-handling for serialization
    // error.
    LOG(ERROR) << "Could not write download entries to file "
               << "because of a serialization issue.";
    return std::string();
  }
  return entries_string;
}

void WriteEntriesToFile(const std::string& entries, base::FilePath file_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DCHECK(!file_path.empty());

  if (!base::ImportantFileWriter::WriteFileAtomically(file_path, entries)) {
    LOG(ERROR) << "Could not write download entries to file: "
               << file_path.value();
  }
}
}  // namespace

InProgressCacheImpl::InProgressCacheImpl(
    const base::FilePath& cache_file_path,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner)
    : file_path_(cache_file_path),
      is_initialized_(false),
      task_runner_(task_runner),
      weak_ptr_factory_(this) {}

InProgressCacheImpl::~InProgressCacheImpl() = default;

void InProgressCacheImpl::Initialize(const base::Closure& callback) {
  // If it's already initialized, just run the callback.
  if (is_initialized_) {
    task_runner_->PostTask(FROM_HERE, callback);
    return;
  }

  pending_actions_.push_back(callback);

  // Initialize |entries_| by reading from file.
  base::PostTaskAndReplyWithResult(
      task_runner_.get(), FROM_HERE,
      base::Bind(&ReadEntriesFromFile, file_path_),
      base::Bind(&InProgressCacheImpl::OnInitialized,
                 weak_ptr_factory_.GetWeakPtr()));
}

void InProgressCacheImpl::OnInitialized(std::unique_ptr<char[]> entries) {
  if (entries != nullptr) {
    if (!entries_.ParseFromArray(entries.get(), sizeof(entries.get()))) {
      // TODO(crbug.com/778425): Get UMA for errors.
      LOG(ERROR) << "Could not read download entries from file "
                 << "because there was a parse failure.";
      return;
    }
  }

  is_initialized_ = true;

  while (!pending_actions_.empty()) {
    auto callback = pending_actions_.front();
    task_runner_->PostTask(FROM_HERE, callback);
    pending_actions_.pop_front();
  }
}

void InProgressCacheImpl::AddOrReplaceEntry(const DownloadEntry& entry) {
  if (!is_initialized_) {
    LOG(ERROR) << "Cache is not initialized, cannot AddOrReplaceEntry.";
    return;
  }

  // Update |entries_|.
  AddOrReplaceEntryInEntries(entries_, entry);

  // Serialize |entries_| and write to file.
  std::string entries_string = EntriesToString(entries_);
  task_runner_->PostTask(FROM_HERE, base::BindOnce(&WriteEntriesToFile,
                                                   entries_string, file_path_));
}

base::Optional<DownloadEntry> InProgressCacheImpl::RetrieveEntry(
    const std::string& guid) {
  if (!is_initialized_) {
    LOG(ERROR) << "Cache is not initialized, cannot RetrieveEntry.";
    return base::nullopt;
  }

  return GetEntryFromEntries(entries_, guid);
}

void InProgressCacheImpl::RemoveEntry(const std::string& guid) {
  if (!is_initialized_) {
    LOG(ERROR) << "Cache is not initialized, cannot RemoveEntry.";
    return;
  }

  // Update |entries_|.
  RemoveEntryFromEntries(entries_, guid);

  // Serialize |entries_| and write to file.
  std::string entries_string = EntriesToString(entries_);
  task_runner_->PostTask(FROM_HERE, base::BindOnce(&WriteEntriesToFile,
                                                   entries_string, file_path_));
}

}  // namespace download

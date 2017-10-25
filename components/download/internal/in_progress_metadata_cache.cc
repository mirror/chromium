// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/in_progress_metadata/in_progress_metadata_cache.h"

#include "base/files/important_file_writer.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"

namespace active_downloads {

InProgressMetadataCache::InProgressMetadataCache(
    const base::FilePath& file_path) {}

InProgressMetadataCache::~InProgressMetadataCache() = default;

// InProgressMetadataCache -----------------------------------------------------

void InProgressMetadataCache::AddOrUpdateMetadata(const DownloadEntry& entry) {
  AddEntryToEntries(entry);
  task_runner_->PostTask(FROM_HERE, base::BindOnce(&WriteEntriesToFile));
}

DownloadEntry* InProgressMetadataCache::RetrieveMetadata(
    const std::string& guid) {
  task_runner_->PostTask(FROM_HERE, base::BindOnce(&DeleteFile));
  return GetEntryFromEntries(guid);
}

void InProgressMetadataCache::RemoveMetadata(const std::string& guid) {
  RemoveEntryFromEntries(guid);
  task_runner_->PostTask(FROM_HERE, base::BindOnce(&WriteEntriesToFile));
}

void InProgressMetadataCache::ClearMetadata() {
  task_runner_->PostTask(FROM_HERE, base::BindOnce(&DeleteFile));
}
}  // namespace active_downloads

// Entry/entries utility functions ---------------------------------------------
int GetIndexFromEntries(const std::string& guid) {
  int size_of_entries = entries_.entry_size();
  for (int i = 0; i < size_of_entries; i++) {
    if (entries_.entry(i).guid() == guid) {
      return i;
    }
  }
  return -1;
}

DownloadEntry* GetEntryFromEntries(const std::string& guid) {
  int entry_index = GetIndexFromEntries(guid);
  return (entry_index > 0) ? entries_.mutable_entry(entry_index) : nullptr;
}

void AddEntryToEntries(metadata_pb::DownloadEntry entry) {
  int entry_index = GetIndexFromEntries(entry.guid());
  if (entry_index > 0) {
    entries_.mutable_entry(i) = entry;
  } else {
    Entry* new_entry = entries_.add_entry();
    new_entry = entry;
  }
}

void RemoveEntryFromEntries(const std::string& guid) {
  int entry_index = GetIndexFromEntries(guid);
  if (entry_index > 0) {
    entry_.mutable_entry()->DeleteSubrange(entry_index, 1);
  }
}

void ClearEntries() {
  entries_.reset();
}

// File utility functions ------------------------------------------------------

void WriteEntriesToFile() {
  if (file_path_.empty()) {
    LOG(ERROR) << "Could not write download entries to file because "
               << "no location was set.";
  }

  std::string entries_string;
  if (!entries_.SerializeToString(&entries_string)) {
    LOG(ERROR) << "Could not write download entries to file because "
               << "serialization failed.";
  }

  if (!base::ImportantFileWriter::WriteFileAtomically(file_path_,
                                                      entries_string)) {
    LOG(ERROR) << "Could not write download entries to file: "
               << file_path_.value();
  }
}

void ReadEntriesFromFile() {
  base::File entries_file(file_path_, File::FLAG_OPEN | File::FLAG_READ);

  if (!entries_file.isValid()) {
    if (entries_file.error_details() == File::FILE_ERROR_NOT_FOUND) {
      LOG(ERROR) << "Could not read download entries from file because "
                 << "file is not found.";
      return;
    }
    LOG(ERROR) << "Could not read download entries from file because "
               << "file is not valid.";
    return;
  }

  base::File::Info info;
  if (!entries_file.GetInfo(&info)) {
    LOG(ERROR) << "Could not read download entries from file because "
               << "get info failed.";
    return;
  }

  if (info.size > INT_MAX) {
    LOG(ERROR) << "Could not read download entries from file because "
               << "file is too big.";
    return;
  }

  const int size = static_cast<int>(info.size);
  std::unique_ptr<char[]> file_data(new char[info.size]);
  if (!entries_file.Read(0, file_data.get(), size)) {
    LOG(ERROR) << "Could not read download entries from file because "
               << "there was a read error.";
    return;
  }

  if (!entries_.ParseFromArray(file_data.get(), size)) {
    LOG(ERROR) << "Could not read download entries from file because "
               << "there was a parse failure.";
    return;
  }
}

void DeleteFile() {
  if (!base::DeleteFile(file_path_, false /* not recursive */)) {
    LOG(ERROR) << "Could not delete download entries from file.";
    return;
  }
}

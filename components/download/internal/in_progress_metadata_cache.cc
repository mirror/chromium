// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/in_progress_metadata/in_progress_metadata_cache.h"
#include "components/download/in_progress_metadata/in_progress_metadata_conversions.h"

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/download/in_progress_metadata/download_entry.h"
#include "components/download/in_progress_metadata/proto/download_entry.pb.h"

namespace active_downloads {

InProgressMetadataCache::InProgressMetadataCache(
    const base::FilePath& file_path,
    const scoped_refptr<base::SequencedTaskRunner> task_runner)
    : file_path_(file_path),
      task_runner_(task_runner),
      weak_ptr_factory_(this) {
  // Initialize |entries_| by reading from file.
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&InProgressMetadataCache::ReadEntriesFromFile,
                            weak_ptr_factory_.GetWeakPtr()));
}

InProgressMetadataCache::~InProgressMetadataCache() = default;

// Public methods --------------------------------------------------------------

void InProgressMetadataCache::AddOrReplaceMetadata(const DownloadEntry& entry) {
  AddEntryToEntries(InProgressMetadataConversions::DownloadEntryToProto(entry));
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&InProgressMetadataCache::WriteEntriesToFile,
                            weak_ptr_factory_.GetWeakPtr()));
}

DownloadEntry InProgressMetadataCache::RetrieveMetadata(
    const std::string& guid) {
  return GetEntryFromEntries(guid);
}

void InProgressMetadataCache::RemoveMetadata(const std::string& guid) {
  RemoveEntryFromEntries(guid);
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&InProgressMetadataCache::WriteEntriesToFile,
                            weak_ptr_factory_.GetWeakPtr()));
}

void InProgressMetadataCache::ClearMetadata() {
  ClearEntries();
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&InProgressMetadataCache::DeleteFile,
                                    weak_ptr_factory_.GetWeakPtr()));
}

// Entry/entries helper functions ----------------------------------------------

int InProgressMetadataCache::GetIndexFromEntries(const std::string& guid) {
  int size_of_entries = entries_.entry_size();
  for (int i = 0; i < size_of_entries; i++) {
    if (entries_.entry(i).guid() == guid) {
      return i;
    }
  }
  return -1;
}

void InProgressMetadataCache::AddEntryToEntries(
    metadata_pb::DownloadEntry entry) {
  int entry_index = GetIndexFromEntries(entry.guid());
  metadata_pb::DownloadEntry* entry_ptr =
      (entry_index < 0) ? entries_.add_entry()
                        : entries_.mutable_entry(entry_index);
  entry_ptr = &entry;
}

DownloadEntry InProgressMetadataCache::GetEntryFromEntries(
    const std::string& guid) {
  int entry_index = GetIndexFromEntries(guid);
  metadata_pb::DownloadEntry* entry_ptr =
      (entry_index < 0) ? nullptr : entries_.mutable_entry(entry_index);
  return InProgressMetadataConversions::DownloadEntryFromProto(*entry_ptr);
}

void InProgressMetadataCache::RemoveEntryFromEntries(const std::string& guid) {
  int entry_index = GetIndexFromEntries(guid);
  if (entry_index > 0) {
    entries_.mutable_entry()->DeleteSubrange(entry_index, 1);
  }
}

void InProgressMetadataCache::ClearEntries() {
  entries_.clear_entry();
}

// File helper functions -------------------------------------------------------

void InProgressMetadataCache::WriteEntriesToFile() {
  DCHECK(!file_path_.empty());

  std::string entries_string;
  DCHECK(entries_.SerializeToString(&entries_string));

  if (!base::ImportantFileWriter::WriteFileAtomically(file_path_,
                                                      entries_string)) {
    LOG(ERROR) << "Could not write download entries to file: "
               << file_path_.value();
  }
}

void InProgressMetadataCache::ReadEntriesFromFile() {
  // Check validity of file.
  base::File entries_file(file_path_,
                          base::File::FLAG_OPEN | base::File::FLAG_READ);
  DCHECK(entries_file.IsValid());

  // Get file info.
  base::File::Info info;
  DCHECK(entries_file.GetInfo(&info));
  DCHECK(info.size <= INT_MAX);

  // Read and parse file.
  const int size = static_cast<int>(info.size);
  std::unique_ptr<char[]> file_data(new char[info.size]);
  DCHECK(entries_file.Read(0, file_data.get(), size));
  DCHECK(entries_.ParseFromArray(file_data.get(), size));
}

void InProgressMetadataCache::DeleteFile() {
  DCHECK(base::DeleteFile(file_path_, false /* not recursive */));
}

}  // namespace active_downloads

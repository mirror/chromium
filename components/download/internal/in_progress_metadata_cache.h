// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_IN_PROGRESS_METADATA_CACHE_H_
#define COMPONENTS_DOWNLOAD_IN_PROGRESS_METADATA_CACHE_H_

#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "components/download/in_progress_metadata/download_entry.h"
#include "components/download/in_progress_metadata/proto/download_entry.pb.h"

namespace active_downloads {

// InProgressMetadataCache provides a write-through cache that persists
// information related to an in-progress download such as request origin, retry
// count, resumption parameters etc to the disk. The entries are written to disk
// right away.
class InProgressMetadataCache {
 public:
  InProgressMetadataCache(
      const base::FilePath& file_path,
      const scoped_refptr<base::SequencedTaskRunner> task_runner);
  ~InProgressMetadataCache();

  // Adds an entry.
  void AddOrReplaceMetadata(const DownloadEntry& entry);

  // Retrieves an existing entry.
  DownloadEntry RetrieveMetadata(const std::string& guid);

  // Remove an entry.
  void RemoveMetadata(const std::string& guid);

  // Clear all metadata.
  void ClearMetadata();

 private:
  metadata_pb::DownloadEntries entries_;
  base::FilePath file_path_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::WeakPtrFactory<InProgressMetadataCache> weak_ptr_factory_;

  // Helper functions to handle entry/entries related logic.
  int GetIndexFromEntries(const std::string& guid);
  void AddEntryToEntries(metadata_pb::DownloadEntry entry);
  DownloadEntry GetEntryFromEntries(const std::string& guid);
  void RemoveEntryFromEntries(const std::string& guid);
  void ClearEntries();

  // Helper functions to handle read/write related logic.
  void WriteEntriesToFile();
  void ReadEntriesFromFile();
  void DeleteFile();

  DISALLOW_COPY_AND_ASSIGN(InProgressMetadataCache);
};

}  // namespace active_downloads

#endif  // COMPONENTS_DOWNLOAD_IN_PROGRESS_METADATA_CACHE_H_

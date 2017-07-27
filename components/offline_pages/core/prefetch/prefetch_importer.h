// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_IMPORTER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_IMPORTER_H_

#include "base/files/file_path.h"
#include "base/strings/string16.h"
#include "components/offline_pages/core/client_id.h"
#include "url/gurl.h"

namespace offline_pages {

// Interface to import the downloaded archive such that it can be rendered as
// offline page.
class PrefetchImporter {
 public:
  using CompletedCallback =
      base::Callback<void(bool success, int64_t offline_id)>;

  // Describes all the info needed to import an archive.
  struct ArchiveInfo {
    ArchiveInfo();
    ArchiveInfo(const ArchiveInfo& other);
    ~ArchiveInfo();

    int64_t offline_id = 0;
    ClientId client_id;
    GURL url;
    GURL final_archived_url;
    base::string16 title;
    base::FilePath file_path;
    int64_t file_size = 0;
  };

  virtual ~PrefetchImporter() = default;

  // Imports the downloaded archive by moving the file into archive directory
  // and creating an entry in the offline metadata database.
  virtual void ImportArchive(const ArchiveInfo& info,
                             const CompletedCallback& callback) = 0;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_IMPORTER_H_

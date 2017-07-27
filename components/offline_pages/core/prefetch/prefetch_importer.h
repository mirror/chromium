// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_IMPORTER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_IMPORTER_H_

namespace offline_pages {

struct PrefetchItem;

// Interface to import the downloaded archive such that it can be rendered as
// offline page.
class PrefetchImporter {
 public:
  using CompletedCallback =
      base::Callback<void(bool success, int64_t offline_id)>;

  virtual ~PrefetchImporter() = default;

  // Imports the downloaded archive by moving the file into archive directory
  // and creating an entry in the offline metadata database.
  virtual void ImportFile(const PrefetchItem& item,
                          const CompletedCallback& callback) = 0;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_IMPORTER_H_

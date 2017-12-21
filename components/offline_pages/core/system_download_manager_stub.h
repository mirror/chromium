// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_SYSTEM_DOWNLOAD_MANAGER_STUB_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_SYSTEM_DOWNLOAD_MANAGER_STUB_H_

#include "components/offline_pages/core/system_download_manager.h"

namespace offline_pages {

// Interface of a class responsible for interacting with the OS level download
// manager
class SystemDownloadManagerStub : public SystemDownloadManager {
 public:
  SystemDownloadManagerStub(int64_t download_id);
  ~SystemDownloadManagerStub() override;

  // Returns true if a system download manager is available on this
  // platform.
  bool IsDownloadManagerInstalled() override;

  // Returns the download manager ID of the download, which we will place in the
  // offline pages database as part of the offline page item.
  int64_t addCompletedDownload(const std::string& title,
                               const std::string& description,
                               const std::string& path,
                               long length,
                               const std::string& uri,
                               const std::string& referer) override;

  // Returns the number of pages removed.
  int64_t remove(
      const std::vector<int64_t>& android_download_manager_ids) override;

  // accessors for the test to use to check passed parameters.
  std::string title() { return title_; }
  std::string description() { return description_; }
  std::string path() { return path_; }
  std::string uri() { return uri_; }
  std::string referer() { return referer_; }
  long length() { return length_; }

 private:
  int64_t download_id_;
  std::string title_;
  std::string description_;
  std::string path_;
  std::string uri_;
  std::string referer_;
  long length_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_SYSTEM_DOWNLOAD_MANAGER_STUB_H_

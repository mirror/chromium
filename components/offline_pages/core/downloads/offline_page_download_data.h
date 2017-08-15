// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_DOWNLOADS_OFFLINE_PAGE_DOWNLOAD_DATA_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_DOWNLOADS_OFFLINE_PAGE_DOWNLOAD_DATA_H_

#include "base/supports_user_data.h"

namespace net {
class URLRequest;
}

namespace offline_pages {
namespace downloads {

// Stores and passes information from DownloadRequestCore to offline pages code.
// This class is attached to the URLRequest created in DownloadRequestCore in
// order to pass CCT app information from the downloads stack to offline pages
// stack. Once created, this is owned by the URLRequest and will be destroyed
// when the URLRequest is.
class OfflinePageDownloadData : public base::SupportsUserData::Data {
 public:
  ~OfflinePageDownloadData() override {}

  static void Attach(net::URLRequest* request,
                     const std::string& request_origin);
  static OfflinePageDownloadData* Get(const net::URLRequest* request);

  std::string request_origin() const { return request_origin_; }

 private:
  OfflinePageDownloadData() = default;
  static const int kKey;

  std::string request_origin_;
};

}  // namespace downloads
}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CONRE_DOWNLOADS_OFFLINE_PAGE_DOWNLOAD_DATA_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/downloads/offline_page_download_data.h"

#include "base/memory/ptr_util.h"
#include "net/url_request/url_request.h"

namespace offline_pages {
namespace downloads {

// static
const int OfflinePageDownloadData::kKey = 0;

// static
void OfflinePageDownloadData::Attach(net::URLRequest* request,
                                     const std::string& request_origin) {
  auto request_data = base::WrapUnique(new OfflinePageDownloadData());
  request_data->request_origin_ = request_origin;
  request->SetUserData(&kKey, std::move(request_data));
}

// static
OfflinePageDownloadData* OfflinePageDownloadData::Get(
    const net::URLRequest* request) {
  return static_cast<OfflinePageDownloadData*>(request->GetUserData(&kKey));
}

}  // namespace downloads
}  // namespace offline_pages

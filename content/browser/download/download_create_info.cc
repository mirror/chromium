// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_create_info.h"

#include <string>

#include "base/format_macros.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "content/public/browser/download_item.h"

namespace content {

DownloadCreateInfo::DownloadCreateInfo(
    const base::Time& start_time,
    const net::NetLogWithSource& net_log,
    std::unique_ptr<DownloadSaveInfo> save_info)
    : download_id(DownloadItem::kInvalidId),
      request(new DownloadItem::RequestInfo),
      response(new DownloadItem::ResponseInfo),
      save_info(std::move(save_info)),
      request_net_log(net_log) {
  request->start_time = start_time;
}

DownloadCreateInfo::DownloadCreateInfo()
    : DownloadCreateInfo(base::Time::Now(),
                         net::BoundNetLog(),
                         base::WrapUnique(new DownloadSaveInfo)) {}

DownloadCreateInfo::~DownloadCreateInfo() {
  DCHECK(!this->request);
  DCHECK(!this->response);
  DCHECK(!this->save_info);
  DCHECK(!this->request_handle);
}

}  // namespace content

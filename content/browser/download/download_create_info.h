// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_CREATE_INFO_H_
#define CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_CREATE_INFO_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "content/browser/download/download_file.h"
#include "content/browser/download/download_request_handle.h"
#include "content/common/content_export.h"
#include "content/public/browser/download_interrupt_reasons.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_save_info.h"
#include "net/log/net_log_with_source.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace content {

// Used for informing the download manager of a new download, since we don't
// want to pass |DownloadItem|s between threads.
struct CONTENT_EXPORT DownloadCreateInfo {
  DownloadCreateInfo(const base::Time& start_time,
                     const net::NetLogWithSource& net_log,
                     std::unique_ptr<DownloadSaveInfo> save_info);
  DownloadCreateInfo();
  ~DownloadCreateInfo();

  // The ID of the download.
  uint32_t download_id;

  // If the download is initially created in an interrupted state (because the
  // response was in error), then |result| would be something other than
  // INTERRUPT_REASON_NONE.
  DownloadInterruptReason result;

  // Request data.
  std::unique_ptr<DownloadItem::RequestInfo> request;

  // Response data.
  std::unique_ptr<DownloadItem::ResponseInfo> response;

  // The download file save info.
  std::unique_ptr<DownloadSaveInfo> save_info;

  // The handle to the URLRequest sourcing this download.
  std::unique_ptr<DownloadRequestHandleInterface> request_handle;

  // The request's |NetLogWithSource|, for "source_dependency" linking with the
  // download item's.
  const net::NetLogWithSource request_net_log;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadCreateInfo);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_CREATE_INFO_H_

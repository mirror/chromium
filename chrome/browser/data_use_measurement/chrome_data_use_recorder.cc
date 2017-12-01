// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/data_use_measurement/chrome_data_use_recorder.h"
#include "content/public/browser/resource_request_info.h"
#include "ui/base/page_transition_types.h"

namespace data_use_measurement {

ChromeDataUseRecorder::ChromeDataUseRecorder(DataUse::TrafficType traffic_type)
    : DataUseRecorder(traffic_type),
      main_frame_id_(-1, -1),
      media_data_use_(traffic_type) {
  set_page_transition(ui::PAGE_TRANSITION_LAST_CORE + 1);
}

ChromeDataUseRecorder::~ChromeDataUseRecorder() {}

void ChromeDataUseRecorder::OnNetworkBytesSent(net::URLRequest* request,
                                               int64_t bytes_sent) {
  DataUseRecorder::OnNetworkBytesSent(request, bytes_sent);
  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(request);

  if (!request_info ||
      request_info->GetResourceType() != content::RESOURCE_TYPE_MEDIA)
    return;

  media_data_use_.IncrementTotalBytes(0, bytes_sent);
}

void ChromeDataUseRecorder::OnNetworkBytesReceived(net::URLRequest* request,
                                                   int64_t bytes_received) {
  DataUseRecorder::OnNetworkBytesReceived(request, bytes_received);

  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(request);

  if (!request_info ||
      request_info->GetResourceType() != content::RESOURCE_TYPE_MEDIA)
    return;

  media_data_use_.IncrementTotalBytes(bytes_received, 0);
}

}  // namespace data_use_measurement

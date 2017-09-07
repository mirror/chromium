// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/downloads/offline_item_conversions.h"

#include "base/strings/utf_string_conversions.h"
#include "components/offline_pages/core/background/save_page_request.h"
#include "components/offline_pages/core/offline_page_item.h"

namespace offline_pages {

OfflineItem CreateOfflineItem(const offline_pages::OfflinePageItem& page) {
  OfflineItem item;
  item.id = ContentId(kOfflinePageNamespace, page.client_id.id);
  item.title = base::UTF16ToUTF8(page.title);
  item.filter = offline_items_collection::OfflineItemFilter::FILTER_PAGE;
  item.state = offline_items_collection::OfflineItemState::COMPLETE;
  item.total_size_bytes = page.file_size;
  item.creation_time = page.creation_time;
  item.file_path = page.file_path.value();
  item.mime_type = "text/html";
  item.page_url = page.url;
  item.original_url = page.original_url;
  item.progress.value = 100;
  item.progress.max = 100;
  item.progress.unit =
      offline_items_collection::OfflineItemProgressUnit::PERCENTAGE;

  return item;
}

OfflineItem CreateOfflineItem(const offline_pages::SavePageRequest& request) {
  OfflineItem item;
  item.id = ContentId(kOfflinePageNamespace, request.client_id().id);
  item.filter = offline_items_collection::OfflineItemFilter::FILTER_PAGE;
  item.creation_time = request.creation_time();
  item.total_size_bytes = -1L;
  item.received_bytes = 0;
  item.mime_type = "text/html";
  item.page_url = request.url();
  item.original_url = request.original_url();
  switch (request.request_state()) {
    case offline_pages::SavePageRequest::RequestState::AVAILABLE:
      item.state = offline_items_collection::OfflineItemState::PENDING;
      break;
    case offline_pages::SavePageRequest::RequestState::OFFLINING:
      item.state = offline_items_collection::OfflineItemState::IN_PROGRESS;
      break;
    case offline_pages::SavePageRequest::RequestState::PAUSED:
      item.state = offline_items_collection::OfflineItemState::PAUSED;
      break;
  }

  item.progress.value = 0;
  item.progress.unit =
      offline_items_collection::OfflineItemProgressUnit::PERCENTAGE;

  return item;
}

}  // namespace offline_pages

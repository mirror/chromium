// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_DOWNLOADS_OFFLINE_ITEM_CONVERSIONS_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_DOWNLOADS_OFFLINE_ITEM_CONVERSIONS_H_

#include "components/offline_items_collection/core/offline_item.h"

const char kOfflinePageNamespace[] = "LEGACY_OFFLINE_PAGE";

using ContentId = offline_items_collection::ContentId;
using OfflineItem = offline_items_collection::OfflineItem;

namespace offline_pages {

struct OfflinePageItem;
class SavePageRequest;

OfflineItem CreateOfflineItem(const offline_pages::OfflinePageItem& page);

OfflineItem CreateOfflineItem(const offline_pages::SavePageRequest& request);

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_DOWNLOADS_OFFLINE_ITEM_CONVERSIONS_H_

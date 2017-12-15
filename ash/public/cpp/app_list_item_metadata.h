// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_APP_LIST_ITEM_METADATA_H_
#define ASH_PUBLIC_CPP_APP_LIST_ITEM_METADATA_H_

#include <string>
#include <vector>

#include "ash/public/cpp/ash_public_export.h"
#include "components/sync/model/string_ordinal.h"

namespace ash {

// A structure holding the common information of AppListItem and
// ChromeAppListItem, which is sent between ash and chrome representing
// an app list item.
struct ASH_PUBLIC_EXPORT AppListItemMetadata {
  explicit AppListItemMetadata(const std::string& item_id);
  void UpdateWith(const AppListItemMetadata& item_data);
  ~AppListItemMetadata();
  AppListItemMetadata& operator=(const AppListItemMetadata& other);

  std::string id;
  std::string name;
  std::string folder_id;
  syncer::StringOrdinal position;
  bool is_folder = false;
};

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_APP_LIST_ITEM_METADATA_H_

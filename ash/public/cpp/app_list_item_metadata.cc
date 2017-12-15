// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "ash/public/cpp/app_list_item_metadata.h"

namespace ash {

AppListItemMetadata::AppListItemMetadata(const std::string& item_id)
    : id(item_id) {}

AppListItemMetadata::~AppListItemMetadata() = default;

AppListItemMetadata& AppListItemMetadata::operator=(
    const AppListItemMetadata& other) {
  DCHECK(id.empty() || id == other.id);
  name = other.name;
  folder_id = other.folder_id;
  position = other.position;
  is_folder = other.is_folder;
  return *this;
}

}  // namespace ash

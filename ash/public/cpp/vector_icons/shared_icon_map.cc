// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/vector_icons/shared_icon_map.h"

#include "ash/public/cpp/vector_icons/vector_icons.h"

namespace ash {

void RegisterNotificationIcons() {
  const std::vector<std::pair<const gfx::VectorIcon&, const char*>>
      kSharedIconIds = {
          {kNotificationEndOfSupportIcon, kNotificationEndOfSupportIconId}};

  message_center::AssociateIconsToIds(kSharedIconIds);
}

}  // namespace ash

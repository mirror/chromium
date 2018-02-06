// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_VIEWS_NOTIFICATION_VIEW_COMMON_H_
#define UI_MESSAGE_CENTER_VIEWS_NOTIFICATION_VIEW_COMMON_H_

namespace message_center {

// Character limit = pixels per line * line limit / min. pixels per character.
const int kMinPixelsPerTitleCharacter = 4;

constexpr size_t kMessageCharacterLimit =
    kNotificationWidth * kMessageExpandedLineLimit / 3;

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_VIEWS_NOTIFICATION_VIEW_COMMON_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/views/constants.h"

#include "ui/message_center/message_center_style.h"

namespace message_center {

size_t GetMessageCharacterLimit() {
  return message_center::GetNotificationWidth() *
         message_center::kMessageExpandedLineLimit / 3;
}

size_t GetContextMessageCharacterLimit() {
  return message_center::GetNotificationWidth() *
         message_center::kContextMessageLineLimit / 3;
}

}  // namespace message_center

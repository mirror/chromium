// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_MESSAGE_CENTER_DELEGATE_H_
#define UI_MESSAGE_CENTER_MESSAGE_CENTER_DELEGATE_H_

#include "ui/message_center/message_center_export.h"

namespace message_center {

class MessageCenterDelegate {
 public:
  virtual ~MessageCenterDelegate() = default;

  // "Chromium OS" or "Chrome OS" in the current locale.
  // Return empty string if not on these platforms.
  virtual base::string16 GetProductOSName() const = 0;
};

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_MESSAGE_CENTER_DELEGATE_H_

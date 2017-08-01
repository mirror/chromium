// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_MESSAGE_CENTER_MESSAGE_CENTER_DELEGATE_IMPL_H_
#define CHROME_BROWSER_CHROMEOS_MESSAGE_CENTER_MESSAGE_CENTER_DELEGATE_IMPL_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/message_center/message_center_delegate.h"

namespace chromeos {

class MessageCenterDelegateImpl : public message_center::MessageCenterDelegate {
 public:
  MessageCenterDelegateImpl();

  ~MessageCenterDelegateImpl() override;

  base::string16 GetProductOSName() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MessageCenterDelegateImpl);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_MESSAGE_CENTER_MESSAGE_CENTER_DELEGATE_IMPL_H_

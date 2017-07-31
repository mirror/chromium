// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/message_center/message_center_delegate_impl.h"

#include "chrome/grit/chromium_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace chromeos {

MessageCenterDelegateImpl::~MessageCenterDelegateImpl() = default;

base::string16 MessageCenterDelegateImpl::GetProductOSName() {
  return l10n_util::GetStringUTF16(IDS_PRODUCT_OS_NAME);
}

}  // namespace chromeos

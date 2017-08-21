// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INFOBARS_CORE_NOTIFY_INFOBAR_DELEGATE_H_
#define COMPONENTS_INFOBARS_CORE_NOTIFY_INFOBAR_DELEGATE_H_

#include "components/infobars/core/infobar_delegate.h"

// TODO
namespace infobars {
class NotifyInfoBarDelegate : public InfoBarDelegate {
 public:
  virtual base::string16 GetDescription() const = 0;
  virtual base::string16 GetShortDescription() const = 0;
  virtual base::string16 GetFeaturedLinkText() const = 0;
  virtual void OnLinkTapped() = 0;
};
}  // namespace infobars

#endif  // COMPONENTS_INFOBARS_CORE_NOTIFY_INFOBAR_DELEGATE_H_

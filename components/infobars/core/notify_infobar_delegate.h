// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INFOBARS_CORE_NOTIFY_INFOBAR_DELEGATE_H_
#define COMPONENTS_INFOBARS_CORE_NOTIFY_INFOBAR_DELEGATE_H_

#include "components/infobars/core/infobar_delegate.h"

// Defines the callbacks and appearance for a NotifyInfoBar.
// TODO(crbug.com/754754) The InfoBar using this currently only exists for
// Android. (see //c/b/ui/android/NotifyInfoBarAndroid)
namespace infobars {
class NotifyInfoBarDelegate : public InfoBarDelegate {
 public:
  // The full description of the notification, visible when the infobar is
  // expanded.
  virtual base::string16 GetDescription() const = 0;

  // The short description of the notification, visible when the infobar is
  // collapsed.
  virtual base::string16 GetShortDescription() const = 0;

  // The text for the link displayed when the infobar is expanded.
  virtual base::string16 GetFeaturedLinkText() const = 0;

  // Called when the featured link is tapped.
  virtual void OnLinkTapped() = 0;
};
}  // namespace infobars

#endif  // COMPONENTS_INFOBARS_CORE_NOTIFY_INFOBAR_DELEGATE_H_

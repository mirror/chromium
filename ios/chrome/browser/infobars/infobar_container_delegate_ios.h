// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_DELEGATE_IOS_H_
#define IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_DELEGATE_IOS_H_

#import <Foundation/Foundation.h>

#include "components/infobars/core/infobar_container.h"

@protocol InfobarContainerStateDelegate;

class InfoBarContainerDelegateIOS
    : public infobars::InfoBarContainer::Delegate {
 public:
  explicit InfoBarContainerDelegateIOS(
      id<InfobarContainerStateDelegate> delegate)
      : delegate_(delegate) {}

  ~InfoBarContainerDelegateIOS() override {}

  SkColor GetInfoBarSeparatorColor() const override;

  int ArrowTargetHeightForInfoBar(
      size_t index,
      const gfx::SlideAnimation& animation) const override;

  void ComputeInfoBarElementSizes(const gfx::SlideAnimation& animation,
                                  int arrow_target_height,
                                  int bar_target_height,
                                  int* arrow_height,
                                  int* arrow_half_width,
                                  int* bar_height) const override;

  void InfoBarContainerStateChanged(bool is_animating) override;

  bool DrawInfoBarArrows(int* x) const override;

 private:
  __weak id<InfobarContainerStateDelegate> delegate_;
};

#endif  // IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_DELEGATE_IOS_H_

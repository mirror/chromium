// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_HOSTED_APP_BUTTON_CONTAINER_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_HOSTED_APP_BUTTON_CONTAINER_H_

#include "base/macros.h"
#include "ui/views/view.h"

class BrowserView;

namespace {
class HostedAppMenuButton;
}

// A container for hosted app buttons in the title bar.
class HostedAppButtonContainer : public views::View {
 public:
  explicit HostedAppButtonContainer(BrowserView* browser_view);
  ~HostedAppButtonContainer() override;

  void SetUseLightImages(bool use_light);

 private:
  HostedAppMenuButton* app_menu_button_;

  DISALLOW_COPY_AND_ASSIGN(HostedAppButtonContainer);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_HOSTED_APP_BUTTON_CONTAINER_H_

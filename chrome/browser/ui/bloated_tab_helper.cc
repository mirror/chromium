// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/bloated_tab_helper.h"

#include <memory>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/process/process.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/bloated_tab_infobar_delegate.h"
#include "chrome/common/channel_info.h"
#include "components/infobars/core/infobar.h"
#include "content/public/browser/browser_child_process_host_iterator.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/process_type.h"
#include "content/public/common/result_codes.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(BloatedTabHelper);

BloatedTabHelper::BloatedTabHelper(content::WebContents* contents)
    : content::WebContentsObserver(contents),
      mode_(Mode::kNone),
      infobar_(nullptr),
      timer_(false, false) {}

BloatedTabHelper::~BloatedTabHelper() {
  timer_.Stop();
}

void BloatedTabHelper::OnRendererBloated(
    content::RenderFrameHost* render_frame_host) {
  if (infobar_) {
    CloseInfoBar();
  }
  Mode new_mode = render_frame_host->GetVisibilityState() ==
                          blink::mojom::PageVisibilityState::kHidden
                      ? Mode::kBackground
                      : Mode::kForeground;
  if (new_mode == Mode::kForeground) {
    mode_ = new_mode;
    ShowInfoBar(mode_);
  } else {
    mode_ = new_mode;
    ReloadTab();
  }
}

void BloatedTabHelper::ReloadTab() {
  web_contents()->GetController().Reload(content::ReloadType::NORMAL, false);
}

void BloatedTabHelper::DidStartLoading() {
  printf("did start loading %p!\n", this);
}

void BloatedTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  printf("did finish navigation %p!\n", this);
  if (mode_ == Mode::kBackground) {
    printf("  > mode is background showing bar\n");
    ShowInfoBar(mode_);
  }
}

void BloatedTabHelper::WasShown() {
  timer_.Stop();
}

void BloatedTabHelper::WasHidden() {
  const int kDelayInSeconds = 3;
  if (mode_ == Mode::kForeground) {
    timer_.Start(
        FROM_HERE, base::TimeDelta::FromSeconds(kDelayInSeconds),
        base::Bind(&BloatedTabHelper::OnTimer, base::Unretained(this)));
  }
}

void BloatedTabHelper::OnTimer() {
  if (mode_ == Mode::kForeground) {
    CloseInfoBar();
    mode_ = Mode::kBackground;
    printf("was hidden reloading tab %p\n", this);
    ReloadTab();
  }
}

void BloatedTabHelper::ShowInfoBar(Mode mode) {
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(web_contents());
  if (!infobar_service)
    return;
  DCHECK(!infobar_);

  infobar_ = BloatedTabInfoBarDelegate::Create(infobar_service, this, mode);
}

void BloatedTabHelper::CloseInfoBar() {
  infobar_->CloseInfoBar();
  infobar_ = nullptr;
}

void BloatedTabHelper::ClosedInfoBar(BloatedTabInfoBarDelegate* infobar) {
  if (infobar_ == infobar) {
    infobar_ = nullptr;
    mode_ = Mode::kNone;
  }
}

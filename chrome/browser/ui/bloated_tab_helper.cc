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
#include "content/public/common/process_type.h"
#include "content/public/common/result_codes.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(BloatedTabHelper);

BloatedTabHelper::BloatedTabHelper(content::WebContents* contents)
    : content::WebContentsObserver(contents), infobar_(nullptr) {}

BloatedTabHelper::~BloatedTabHelper() {}

void BloatedTabHelper::OnRendererBloated(
    content::RenderFrameHost* render_frame_host) {
  ShowBar();
}
void BloatedTabHelper::ReloadTab() {
  CloseBar();
}

void BloatedTabHelper::ShowBar() {
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(web_contents());
  if (!infobar_service)
    return;

  DCHECK(!infobar_);
  infobar_ = BloatedTabInfoBarDelegate::Create(infobar_service, this);
}

void BloatedTabHelper::CloseBar() {
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(web_contents());
  if (infobar_service && infobar_) {
    infobar_service->RemoveInfoBar(infobar_);
    infobar_ = nullptr;
  }
}

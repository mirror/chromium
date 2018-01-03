// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/gpu/three_d_api_observer.h"

#include "chrome/browser/gpu/three_d_api_infobar_delegate.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "content/public/browser/gpu_data_manager.h"

ThreeDAPIObserver::ThreeDAPIObserver() {
  content::GpuDataManager::GetInstance()->AddObserver(this);
}

ThreeDAPIObserver::~ThreeDAPIObserver() {
  content::GpuDataManager::GetInstance()->RemoveObserver(this);
}

void ThreeDAPIObserver::DidBlock3DAPIs(const GURL& top_origin_url,
                                       int render_process_id,
                                       int render_frame_id,
                                       content::ThreeDAPIType requester) {
  content::WebContents* web_contents = tab_util::GetWebContentsByFrameID(
      render_process_id, render_frame_id);
  if (!web_contents)
    return;
  ThreeDAPIInfoBarDelegate::Create(
      InfoBarService::FromWebContents(web_contents), top_origin_url, requester);
}

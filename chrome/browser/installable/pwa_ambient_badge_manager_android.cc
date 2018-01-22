// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/installable/pwa_ambient_badge_manager_android.h"

#include "base/time/time.h"
#include "chrome/browser/installable/installable_manager.h"
#include "chrome/browser/installable/pwa_ambient_badge_infobar_delegate_android.h"
#include "content/public/browser/web_contents.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "url/gurl.h"

namespace {

InstallableParams GetInstallableParams() {
  InstallableParams params;
  params.check_eligibility = true;
  params.valid_primary_icon = true;
  params.valid_badge_icon = true;
  params.valid_manifest = true;
  params.has_worker = true;
  params.wait_for_worker = true;

  return params;
}

}  // anonymous namespace

DEFINE_WEB_CONTENTS_USER_DATA_KEY(PwaAmbientBadgeManagerAndroid);

PwaAmbientBadgeManagerAndroid::~PwaAmbientBadgeManagerAndroid() {}

PwaAmbientBadgeManagerAndroid::PwaAmbientBadgeManagerAndroid(
    content::WebContents* contents)
    : content::WebContentsObserver(contents),
      installable_manager_(InstallableManager::FromWebContents(contents)),
      weak_factory_(this) {
  DCHECK(installable_manager_);
}

void PwaAmbientBadgeManagerAndroid::OnDidFinishInstallableCheck(
    const InstallableData& data) {
  if (data.error_code != NO_ERROR_DETECTED)
    return;

  PwaAmbientBadgeInfoBarDelegateAndroid::Create(web_contents(),
                                                *data.primary_icon);
}

void PwaAmbientBadgeManagerAndroid::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  installable_manager_->GetData(
      GetInstallableParams(),
      base::Bind(&PwaAmbientBadgeManagerAndroid::OnDidFinishInstallableCheck,
                 weak_factory_.GetWeakPtr()));
}

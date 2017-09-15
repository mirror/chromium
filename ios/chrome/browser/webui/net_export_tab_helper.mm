// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/webui/net_export_tab_helper.h"

#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/webui/mail_composer_presenter.h"
#import "ios/web/public/web_state/web_state.h"

@class ShowMailComposerCommand;

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(NetExportTabHelper);

// static
void NetExportTabHelper::CreateForWebState(
    web::WebState* web_state,
    id<MailComposerPresenter> presenter) {
  DCHECK(web_state);
  if (!FromWebState(web_state)) {
    web_state->SetUserData(UserDataKey(),
                           base::WrapUnique(new NetExportTabHelper(presenter)));
  }
}

NetExportTabHelper::NetExportTabHelper(id<MailComposerPresenter> presenter)
    : presenter_(presenter) {
  DCHECK(presenter);
}

NetExportTabHelper::~NetExportTabHelper() = default;

void NetExportTabHelper::ShowMailComposer(ShowMailComposerCommand* context) {
  [presenter_ showMailComposer:context];
}

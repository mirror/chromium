// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/ntp/new_tab_page_tab_helper.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "ios/chrome/browser/ui/ntp/new_tab_page_presenter.h"
#include "ios/web/public/web_state/navigation_context.h"
#include "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(NewTabPageTabHelper);

// static
void NewTabPageTabHelper::CreateForWebState(web::WebState* web_state,
                                            id<NewTabPagePresenter> presenter) {
  DCHECK(web_state);
  if (!FromWebState(web_state)) {
    web_state->SetUserData(
        UserDataKey(),
        base::WrapUnique(new NewTabPageTabHelper(web_state, presenter)));
  }
}

NewTabPageTabHelper::~NewTabPageTabHelper() = default;

NewTabPageTabHelper::NewTabPageTabHelper(web::WebState* web_state,
                                         id<NewTabPagePresenter> presenter)
    : presenter_(presenter) {
  DCHECK(presenter);
  web_state->AddObserver(this);
}

void NewTabPageTabHelper::WebStateDestroyed(web::WebState* web_state) {
  web_state->RemoveObserver(this);
}

void NewTabPageTabHelper::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  if (web_state->GetLastCommittedURL() == GURL("chrome://newtab/")) {
    [presenter_ showNTP:web_state];
  } else {
    [presenter_ hideNTP:web_state];
  }
}

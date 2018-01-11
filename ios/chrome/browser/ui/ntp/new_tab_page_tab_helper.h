// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_TAB_HELPER_H_

#include "base/macros.h"
#include "ios/web/public/web_state/web_state_observer.h"
#include "ios/web/public/web_state/web_state_user_data.h"

@protocol NewTabPagePresenter;

// Foo
class NewTabPageTabHelper : public web::WebStateObserver,
                            public web::WebStateUserData<NewTabPageTabHelper> {
 public:
  ~NewTabPageTabHelper() override;

  // Bar
  static void CreateForWebState(web::WebState* web_state,
                                id<NewTabPagePresenter> presenter);

 private:
  NewTabPageTabHelper(web::WebState* web_state,
                      id<NewTabPagePresenter> presenter);

  // web::WebStateObserver overrides:
  void WebStateDestroyed(web::WebState* web_state) override;
  void DidFinishNavigation(web::WebState* web_state,
                           web::NavigationContext* navigation_context) override;

  // Need more here.

  __weak id<NewTabPagePresenter> presenter_;

  DISALLOW_COPY_AND_ASSIGN(NewTabPageTabHelper);
};

#endif  // IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_TAB_HELPER_H_

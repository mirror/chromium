// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_PRINT_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_WEB_PRINT_TAB_HELPER_H_

#include "ios/web/public/web_state/web_state_observer.h"
#include "ios/web/public/web_state/web_state_user_data.h"

@protocol BrowserCommands;
class GURL;

namespace base {
class DictionaryValue;
}  // namespace base

// Handles print requests from JavaScript window.print.
class PrintTabHelper : public web::WebStateObserver,
                       public web::WebStateUserData<PrintTabHelper> {
 public:
  ~PrintTabHelper() override;

  // Sets the dispatcher.
  void SetDispatcher(id<BrowserCommands> dispatcher);

 private:
  friend class web::WebStateUserData<PrintTabHelper>;

  explicit PrintTabHelper(web::WebState* web_state);

  // web::WebStateObserver overrides:
  void WebStateDestroyed() override;

  // Called when print message is sent by the web page.
  bool OnPrintCommand(const base::DictionaryValue&, const GURL&, bool);

  // Stops handling print requests from the web page.
  void Detach();

  __weak id<BrowserCommands> dispatcher_;
};

#endif  // IOS_CHROME_BROWSER_WEB_PRINT_TAB_HELPER_H_

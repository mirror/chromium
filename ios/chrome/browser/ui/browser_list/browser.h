// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_H_
#define IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_H_

#include <memory>

#include "base/macros.h"
#include "base/supports_user_data.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"

class WebStateList;
class WebStateListDelegate;

@class ChromeBroadcaster;
@class CommandDispatcher;

namespace ios {
class ChromeBrowserState;
}

// Browser holds the state backing a collection of Tabs and the attached
// UI elements (Tab strip, ...).
class Browser : public base::SupportsUserData {
 public:
  Browser(ios::ChromeBrowserState* browser_state,
          WebStateListDelegate* delegate);
  ~Browser() override;

  WebStateList& web_state_list() { return web_state_list_; }
  const WebStateList& web_state_list() const { return web_state_list_; }

  CommandDispatcher* dispatcher() { return dispatcher_; }

  ios::ChromeBrowserState* browser_state() const { return browser_state_; }

  ChromeBroadcaster* broadcaster() { return broadcaster_; }

 private:
  __strong ChromeBroadcaster* broadcaster_;
  __strong CommandDispatcher* dispatcher_;
  ios::ChromeBrowserState* browser_state_;
  WebStateList web_state_list_;

  DISALLOW_COPY_AND_ASSIGN(Browser);
};

#endif  // IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_H_

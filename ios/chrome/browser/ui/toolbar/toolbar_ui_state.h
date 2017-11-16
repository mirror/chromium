// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_UI_STATE_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_UI_STATE_H_

#import "ios/chrome/browser/ui/toolbar/toolbar_ui.h"

// Object conforming to ToolbarUI.  It can be used as a container object for
// the broadcasted value if necessary.
@interface ToolbarUIState : NSObject<ToolbarUI>
@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_UI_STATE_H_

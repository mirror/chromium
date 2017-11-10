// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_COORDINATOR_H_

#import "ios/chrome/browser/chrome_coordinator.h"
#import "ios/chrome/browser/translate/language_selection_handler.h"

@interface LanguageSelectionCoordinator
    : ChromeCoordinator<LanguageSelectionHandler>
@end

#endif  // IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_COORDINATOR_H_

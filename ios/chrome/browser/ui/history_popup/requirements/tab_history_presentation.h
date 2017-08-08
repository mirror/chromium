// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_POPUP_REQUIREMENTS_TAB_HISTORY_PRESENTATION_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_POPUP_REQUIREMENTS_TAB_HISTORY_PRESENTATION_H_

@protocol TabHistoryPresentation
@optional
// Called before presenting the Tab History Popup.
- (void)prepareForTabHistoryPresentation;

// Called after the Tab History Popup was dismissed.
- (void)tabHistoryWasDismissed;
@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_POPUP_REQUIREMENTS_TAB_HISTORY_PRESENTATION_H_

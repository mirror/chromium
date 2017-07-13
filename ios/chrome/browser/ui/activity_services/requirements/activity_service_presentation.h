// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_REQUIREMENTS_ACTIVITY_SERVICE_PRESENTATION_H_
#define IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_REQUIREMENTS_ACTIVITY_SERVICE_PRESENTATION_H_

@protocol ActivityServicePresentation

- (void)showErrorAlertWithStringTitle:(NSString*)title
                              message:(NSString*)message;

- (void)activityServiceDidEndPresenting;

@end

#endif  // IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_REQUIREMENTS_ACTIVITY_SERVICE_PRESENTATION_H_

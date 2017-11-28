// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_STATE_DELEGATE_H_
#define IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_STATE_DELEGATE_H_

#import <Foundation/Foundation.h>

@protocol InfobarContainerStateDelegate

- (void)infoBarContainerStateDidChangeAnimated:(BOOL)animated;

@end

#endif  // IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_STATE_DELEGATE_H_

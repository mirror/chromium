// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_APP_VIEW_UTILS_H_
#define REMOTING_IOS_APP_VIEW_UTILS_H_

#import <UIKit/UIKit.h>

namespace remoting {

UIViewController* TopPresentingVC();

UILayoutGuide* SafeAreaLayoutGuideForView(UIView* view);

}  // namespace remoting

#endif  // REMOTING_IOS_APP_VIEW_UTILS_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_SHARED_CHROME_BROWSER_UI_METRICS_METRICS_TEST_UTIL_H_
#define IOS_SHARED_CHROME_BROWSER_UI_METRICS_METRICS_TEST_UTIL_H_

#import <Foundation/Foundation.h>

namespace metrics_test_util {

// Constructs an NSInvocation for a required instance method defined by
// |selector| in |protocol|.
NSInvocation* GetInvocationForRequiredProtocolInstanceMethod(
    Protocol* protocol,
    SEL selector,
    BOOL isRequiredMethod);

}  // namespace metrics_test_util

#endif  // IOS_SHARED_CHROME_BROWSER_UI_METRICS_METRICS_TEST_UTIL_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cronet/Cronet.h>

#include "components/cronet/ios/test/cronet_test_base.h"

#include "testing/gtest_mac.h"

@interface TestMetricsDelegate : NSObject<CronetMetricsDelegate>
@end

@implementation TestMetricsDelegate
- (void)URLSession:(NSURLSession*)session task:(NSURLSessionTask*)task
                    didFinishCollectingMetrics:(NSURLSessionTaskMetrics*)metrics
    NS_AVAILABLE_IOS(10.0) {
  // This is never actually called, so its definition currently doesn't matter.
}
@end

namespace cronet {

class MetricsTest : public CronetTestBase {
 protected:
  MetricsTest() {}
  ~MetricsTest() override {}
};

TEST_F(MetricsTest, Delegate) {
  if (@available(iOS 10, *)) {
    TestMetricsDelegate* metrics_delegate =
        [[TestMetricsDelegate alloc] init];
    EXPECT_TRUE([Cronet addMetricsDelegate:metrics_delegate]);
    EXPECT_FALSE([Cronet addMetricsDelegate:metrics_delegate]);
    EXPECT_TRUE([Cronet removeMetricsDelegate:metrics_delegate]);
    EXPECT_FALSE([Cronet removeMetricsDelegate:metrics_delegate]);
  }
}

}  // namespace cronet

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cronet/Cronet.h>

#include "components/cronet/ios/test/cronet_test_base.h"
#include "components/cronet/ios/test/start_cronet.h"
#include "components/grpc_support/test/quic_test_server.h"
#include "net/base/mac/url_conversions.h"
#include "net/cert/mock_cert_verifier.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"

#include "testing/gtest_mac.h"

@implementation CronetMetricsDelegate
- (void)didFinishCollectingMetrics:(NSURLSessionTaskTransactionMetrics*)metrics
    NS_AVAILABLE_IOS(10.0) {
  // This is never actually called, so its definition currently doesn't matter.
}
@end

namespace cronet {

class MetricsTest : public ::testing::Test {
 protected:
  MetricsTest() {}
  ~MetricsTest() override {}

  void SetUp() override { StartCronet(grpc_support::GetQuicTestServerPort()); }

  void TearDown() override { [Cronet shutdownForTesting]; }
};

TEST_F(MetricsTest, Delegate) {
  // can this be made cleaner?
  if (@available(iOS 10, *)) {
    CronetMetricsDelegate* metrics_delegate =
        [[CronetMetricsDelegate alloc] init];
    EXPECT_TRUE([Cronet addMetricsDelegate:metrics_delegate]);
    EXPECT_FALSE([Cronet addMetricsDelegate:metrics_delegate]);
    EXPECT_TRUE([Cronet removeMetricsDelegate:metrics_delegate]);
    EXPECT_FALSE([Cronet removeMetricsDelegate:metrics_delegate]);
  }
}

}  // namespace cronet

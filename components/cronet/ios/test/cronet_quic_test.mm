// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cronet/Cronet.h>
#import <Foundation/Foundation.h>

#include <stdint.h>

#include "base/logging.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "components/cronet/ios/test/cronet_test_base.h"
#include "components/cronet/ios/test/start_cronet.h"
#include "components/cronet/ios/test/test_server.h"
#include "components/grpc_support/test/quic_test_server.h"
#include "net/base/mac/url_conversions.h"
#include "net/base/net_errors.h"
#include "net/cert/mock_cert_verifier.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"

#include "url/gurl.h"

namespace cronet {

TEST(QuicTest, InvalidQuicHost) {
  BOOL success =
      [Cronet addQuicHint:@"https://test.example.com/" port:443 altPort:443];

  EXPECT_FALSE(success);
}

TEST(QuicTest, ValidQuicHost) {
  BOOL success = [Cronet addQuicHint:@"test.example.com" port:443 altPort:443];

  EXPECT_TRUE(success);
}
}

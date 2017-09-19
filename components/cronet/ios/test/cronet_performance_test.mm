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

#include "base/strings/stringprintf.h"
#include "url/gurl.h"

namespace cronet {
// const char kUserAgent[] = "CronetTest/1.0.0.0";

struct PerfResult {
  double total;
  double mean;
  double max;
  // also: mean time, median ?, deviation, max, min, etc
};

typedef std::tuple<BOOL, BOOL, BOOL, BOOL> testconfig_t;
auto generator = ::testing::Combine(::testing::Bool(),   // flag for QUIC
                                    ::testing::Bool(),   // flag for HTTP2
                                    ::testing::Bool(),   // flag for AKD4
                                    ::testing::Bool());  // flag for Cronet

class PerfTest : public CronetTestBase,
                 public ::testing::WithParamInterface<testconfig_t> {
 public:
  static std::map<testconfig_t, PerfResult> perftestresults;

  static void TearDownTestCase() {
    NSLog(@"Performance Data:");
    for (auto const& entry : perftestresults) {
      NSLog(@"Quic %i    Http2 %i    AKD4 %i    Cronet %i: %f (%f)",
            std::get<0>(entry.first), std::get<1>(entry.first),
            std::get<2>(entry.first), std::get<3>(entry.first),
            entry.second.mean, entry.second.max);
    }
  }

 protected:
  PerfTest() {}
  ~PerfTest() override {}

  void SetUp() override {
    CronetTestBase::SetUp();
    TestServer::Start();

    NSLog(@"Reinitializing");

    NSLog(@"*** Parameters: %i x %i x %i x %i", std::get<0>(GetParam()),
          std::get<1>(GetParam()), std::get<2>(GetParam()),
          std::get<3>(GetParam()));

    // These are normally called by StartCronet(), but because of the test
    // parameterization we need to call them inline, and not use StartCronet()
    [Cronet setUserAgent:@"CronetTest/1.0.0.0" partial:NO];
    [Cronet setQuicEnabled:std::get<0>(GetParam())];
    [Cronet setHttp2Enabled:std::get<1>(GetParam())];
    [Cronet setAcceptLanguages:@"en-US,en"];
    [Cronet addQuicHint:@"www.gstatic.com" port:443 altPort:443];
    [Cronet enableTestCertVerifierForTesting];
    [Cronet setHttpCacheType:CRNHttpCacheTypeDisabled];
    if (std::get<2>(GetParam()))
      [Cronet setExperimentalOptions:
                  @"{\"QUIC\":{\"connection_options\":\"AKD4\"}}"];

    [Cronet start];

    NSString* rules = base::SysUTF8ToNSString(
        base::StringPrintf("MAP test.example.com 127.0.0.1:%d,"
                           "MAP notfound.example.com ~NOTFOUND",
                           grpc_support::GetQuicTestServerPort()));
    [Cronet setHostResolverRulesForTesting:rules];
    // This is the end of the behaviore normally performed by StartCronet()

    [Cronet registerHttpProtocolHandler];

    NSURLSessionConfiguration* config =
        [NSURLSessionConfiguration ephemeralSessionConfiguration];
    if (std::get<3>(GetParam())) {
      [Cronet installIntoSessionConfiguration:config];
    }
    session_ = [NSURLSession sessionWithConfiguration:config
                                             delegate:delegate_
                                        delegateQueue:nil];
  }

  void TearDown() override {
    TestServer::Shutdown();

    [Cronet stopNetLog];
    [Cronet shutdownForTesting];
    CronetTestBase::TearDown();
  }

  NSURLSession* session_;
};

// this has to be out-of-line for compiler-coaxing reasons
std::map<testconfig_t, PerfResult> PerfTest::perftestresults;

TEST_P(PerfTest, NSURLSessionReceivesImageLoop) {
  int iterations = 50;
  long size = 19307439;
  LOG(INFO) << "Downloading " << size << " bytes " << iterations << " times.";
  NSTimeInterval elapsed_total = 0;
  NSTimeInterval elapsed_max = 0;

  NSURL* url = net::NSURLWithGURL(
      GURL("https://www.gstatic.com/chat/hangouts/bg/davec.jpg"));

  [delegate_ reset];
  for (int i = 0; i < iterations; ++i) {
    //__block BOOL block_used = NO;
    NSURLSessionDataTask* task = [session_ dataTaskWithURL:url];
    [Cronet setRequestFilterBlock:^(NSURLRequest* request) {
      // block_used = YES;
      EXPECT_EQ([request URL], url);
      // return std::get<3>(GetParam());
      return YES;
    }];

    NSDate* start = [NSDate date];
    StartDataTaskAndWaitForCompletion(task);
    //[task resume];
    //[delegate_ waitForDone];

    NSTimeInterval elapsed = -[start timeIntervalSinceNow];
    elapsed_total += elapsed;
    if (elapsed > elapsed_max)
      elapsed_max = elapsed;

    if ([delegate_ error]) {
      LOG(WARNING) << "Request failed during performance testing.";
    }
  }

  // //EXPECT_TRUE(block_used);
  // EXPECT_EQ(nil, [delegate_ error]);
  // EXPECT_EQ(size, [delegate_ totalBytesReceived]);

  LOG(INFO) << "Elapsed Total:" << elapsed_total * 1000 << "ms";

  // perftestresults[GetParam()] = elapsed * 1000;
  //[perftestresults setObject:[NSString stringWithFormat:@"%f", elapsed]
  // forKey:
  //    [NSString stringWithFormat:@"%i", std::get<0>(GetParam())]];
  perftestresults[GetParam()] = {elapsed_total, elapsed_total / iterations,
                                 elapsed_max};

  // LOG(INFO) << "Elapsed Average:" << elapsed_avg * 1000 / iterations
  //          << "ms Max:" << elapsed_max * 1000 << "ms";
}

INSTANTIATE_TEST_CASE_P(Loops, PerfTest, generator);
}

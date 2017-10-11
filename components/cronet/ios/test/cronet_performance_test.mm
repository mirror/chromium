// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cronet/Cronet.h>
#import <Foundation/Foundation.h>

#include <stdint.h>

#include "base/logging.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "components/cronet/ios/test/cronet_perf_test_base.h"
#include "components/cronet/ios/test/start_cronet.h"
#include "components/cronet/ios/test/test_server.h"
#include "components/grpc_support/test/quic_test_server.h"
#include "net/base/mac/url_conversions.h"
#include "net/base/net_errors.h"
#include "net/cert/mock_cert_verifier.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "url/gurl.h"

#define TEST_ITERATIONS 50
#define USE_EXTERNAL_URL true
#define EXTERNAL_SIZE 19307439
#define EXTERNAL_URL "https://www.gstatic.com/chat/hangouts/bg/davec.jpg"

namespace cronet {

struct PerfResult {
  double total;
  double mean;
  double max;
  int failed_requests;
};

struct TestConfig {
  BOOL quic;
  BOOL http2;
  BOOL akd4;
  BOOL cronet;
};

bool operator<(TestConfig a, TestConfig b) {
  return std::tie(a.quic, a.http2, a.akd4, a.cronet) <
         std::tie(b.quic, b.http2, b.akd4, b.cronet);
}

std::set<TestConfig> test_combinations = {
    //  QUIC   HTTP2  AKD4   Cronet
     {  false, false, false, false, },
     {  false, false, false, true,  },
     {  false, true,  false, true,  },
     {  true,  false, false, true,  },
     {  true,  false, true,  true,  },
};
auto generator = ::testing::ValuesIn(test_combinations);

class PerfTest : public CronetPerfTestBase,
                 public ::testing::WithParamInterface<TestConfig> {
 public:
  static std::map<TestConfig, PerfResult> perftestresults;

  static void TearDownTestCase() {
    NSString* filename = [NSString
        stringWithFormat:@"performance_metrics-%@.txt",
                         [[NSDate date]
                             descriptionWithLocale:[NSLocale currentLocale]]];
    NSString* path =
        [[[[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory
                                                  inDomains:NSUserDomainMask]
            lastObject] URLByAppendingPathComponent:filename] path];

    NSMutableString* dates = [NSMutableString stringWithCapacity:0];

    LOG(INFO) << "Performance Data:";
    for (auto const& entry : perftestresults) {
      [dates appendFormat:
                 @"Quic %i    Http2 %i    AKD4 %i    Cronet %i: Mean: %fs Max: "
                 @"%fs with %i fails.\n",
                 entry.first.quic, entry.first.http2, entry.first.akd4,
                 entry.first.cronet, entry.second.mean, entry.second.max,
                 entry.second.failed_requests];

      LOG(INFO) << "Quic " << entry.first.quic << "   Http2 "
                << entry.first.http2 << "    AKD4 " << entry.first.akd4
                << "    Cronet " << entry.first.cronet << ": "
                << "    Mean: " << entry.second.mean
                << "s Max: " << entry.second.max << "s with "
                << entry.second.failed_requests << " fails.";
    }

    NSData* filedata = [dates dataUsingEncoding:NSUTF8StringEncoding];
    [[NSFileManager defaultManager] createFileAtPath:path
                                            contents:filedata
                                          attributes:nil];
  }

 protected:
  PerfTest() {}
  ~PerfTest() override {}

  void SetUp() override {
    CronetPerfTestBase::SetUp();
    TestServer::Start();

    LOG(INFO) << "*** Parameters: " << GetParam().quic << " x "
              << GetParam().http2 << " x " << GetParam().akd4 << " x "
              << GetParam().cronet;

    // These are normally called by StartCronet(), but because of the test
    // parameterization we need to call them inline, and not use StartCronet()
    [Cronet setUserAgent:@"CronetTest/1.0.0.0" partial:NO];
    [Cronet setQuicEnabled:GetParam().quic];
    [Cronet setHttp2Enabled:GetParam().http2];
    [Cronet setAcceptLanguages:@"en-US,en"];
    [Cronet addQuicHint:@"www.gstatic.com" port:443 altPort:443];
    [Cronet enableTestCertVerifierForTesting];
    [Cronet setHttpCacheType:CRNHttpCacheTypeDisabled];
    if (GetParam().akd4)
      [Cronet setExperimentalOptions:
                  @"{\"QUIC\":{\"connection_options\":\"AKD4\"}}"];

    [Cronet start];

    NSString* rules = base::SysUTF8ToNSString(
        base::StringPrintf("MAP test.example.com 127.0.0.1:%d,"
                           "MAP notfound.example.com ~NOTFOUND",
                           grpc_support::GetQuicTestServerPort()));
    [Cronet setHostResolverRulesForTesting:rules];
    // This is the end of the behaviore normally performed by StartCronet()

    NSURLSessionConfiguration* config =
        [NSURLSessionConfiguration ephemeralSessionConfiguration];
    if (GetParam().cronet) {
      [Cronet registerHttpProtocolHandler];
      [Cronet installIntoSessionConfiguration:config];
    } else {
      [Cronet unregisterHttpProtocolHandler];
    }
    session_ = [NSURLSession sessionWithConfiguration:config
                                             delegate:delegate_
                                        delegateQueue:nil];
  }

  void TearDown() override {
    TestServer::Shutdown();

    [Cronet shutdownForTesting];
    CronetPerfTestBase::TearDown();
  }

  NSURLSession* session_;
};

// this has to be out-of-line for compiler-coaxing reasons
std::map<TestConfig, PerfResult> PerfTest::perftestresults;

TEST_P(PerfTest, NSURLSessionReceivesImageLoop) {
  int iterations = TEST_ITERATIONS;
  int failed_iterations = 0;
  long size = EXTERNAL_SIZE;
  LOG(INFO) << "Downloading " << size << " bytes " << iterations << " times.";
  NSTimeInterval elapsed_total = 0;
  NSTimeInterval elapsed_max = 0;

  NSURL* url;
  if (USE_EXTERNAL_URL) {
    url = net::NSURLWithGURL(GURL(EXTERNAL_URL));
  } else {
    url = net::NSURLWithGURL(GURL(TestServer::PrepareBigDataURL(size)));
  }

  [delegate_ reset];
  for (int i = 0; i < iterations; ++i) {
    __block BOOL block_used = NO;
    NSURLSessionDataTask* task = [session_ dataTaskWithURL:url];
    [Cronet setRequestFilterBlock:^(NSURLRequest* request) {
      block_used = YES;
      EXPECT_EQ([request URL], url);
      return YES;
    }];

    NSDate* start = [NSDate date];
    // StartDataTaskAndWaitForCompletion(task);
    [delegate_ reset];
    [task resume];
    [delegate_ waitForDone];

    EXPECT_EQ([delegate_ expectedContentLength],
              [delegate_ totalBytesReceived]);
    // EXPECT_GT([delegate_ totalBytesReceived], 100000);

    NSTimeInterval elapsed = -[start timeIntervalSinceNow];

    if ([delegate_ error]) {
      LOG(WARNING) << "Request failed during performance testing.";
      LOG(WARNING) << [[delegate_ error] localizedDescription];
      failed_iterations++;
    } else {
      elapsed_total += elapsed;
      if (elapsed > elapsed_max)
        elapsed_max = elapsed;
    }

    EXPECT_EQ(block_used, GetParam().cronet);
  }

  LOG(INFO) << "Elapsed Total:" << elapsed_total * 1000 << "ms";

  perftestresults[GetParam()] = {elapsed_total, elapsed_total / iterations,
                                 elapsed_max};

  if (!USE_EXTERNAL_URL) {
    TestServer::ReleaseBigDataURL();
  }
}

INSTANTIATE_TEST_CASE_P(Loops, PerfTest, generator);
}

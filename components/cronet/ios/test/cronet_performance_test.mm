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

// struct PerfTestConfig {
//    BOOL QuicEnabled, // this should be enum
//    // ...
//    BOOL Parallel, // number of requests in parallel?
//    int iterations, // parallel requests * iterations = requests total
//}
//
// struct PerfResult {
//   int total_time;
//   // also: mean time, median ?, deviation, max, min, etc
//}

typedef struct payload_s {
  int size;
  NSURL* url;
} payload_t;

class PerfTestF : public CronetTestBase {
 protected:
  PerfTestF() {}
  ~PerfTestF() override {}

  void SetUp() override {
    CronetTestBase::SetUp();
    TestServer::Start();

    NSLog(@"Reinitializing");

    // Cronet(netlog+quic):             Elapsed Average:   16738.7ms   Max:
    // 17617.1ms Cronet(nonetlog+quic):           Elapsed Average:    1976.75ms
    // Max:  3253.72ms Cronet(nonetlog+noquic):         Elapsed Average:
    // 829.827ms Max:  1221.86ms Cronet(nonetlog+quic+nohttp2):   Elapsed
    // Average:    1947.5ms   Max:  2291.02ms Cronet(nonetlog+noquic+nohttp2):
    // Elapsed Average:     848.385ms Max:  1321.86ms

    // Platform(netlog+?quic):   Elapsed Average:    1177.87ms  Max:  1505.52ms
    // Platform(nonetlog+?quic): Elapsed Average:1006.13ms Max:1779.11ms

    //[Cronet setRequestFilterBlock:^(NSURLRequest* request) {
    //  return YES; // 96634 ms
    //  //return NO; // 96404 ms
    //}];

    // StartCronet
    [Cronet setUserAgent:@"CronetTest/1.0.0.0" partial:NO];
    [Cronet setQuicEnabled:true];
    [Cronet setHttp2Enabled:NO];
    [Cronet setAcceptLanguages:@"en-US,en"];
    //[Cronet addQuicHint:@"test.example.com" port:443 altPort:443];
    [Cronet addQuicHint:@"www.gstatic.com" port:443 altPort:443];
    [Cronet enableTestCertVerifierForTesting];
    [Cronet setHttpCacheType:CRNHttpCacheTypeDisabled];
    [Cronet
        setExperimentalOptions:@"{\"QUIC\":{\"connection_options\":\"AKD4\"}}"];

    [Cronet start];

    NSString* rules = base::SysUTF8ToNSString(
        base::StringPrintf("MAP test.example.com 127.0.0.1:%d,"
                           "MAP notfound.example.com ~NOTFOUND",
                           grpc_support::GetQuicTestServerPort()));
    [Cronet setHostResolverRulesForTesting:rules];
    // StartedCronet

    //[Cronet startNetLogToFile:@"netlog7.json" logBytes:NO];
    [Cronet registerHttpProtocolHandler];

    NSURLSessionConfiguration* config =
        [NSURLSessionConfiguration ephemeralSessionConfiguration];
    [Cronet installIntoSessionConfiguration:config];
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

typedef std::tuple<BOOL, BOOL, BOOL, BOOL> testconfig_t;
auto generator = ::testing::Combine(::testing::Bool(),
                                    ::testing::Bool(),
                                    ::testing::Bool(),
                                    ::testing::Bool());

class PerfTest : public CronetTestBase,
                 public ::testing::WithParamInterface<testconfig_t> {
 public:
  // static member of PerfTest
  static std::map<testconfig_t, double> perftestresults;

  static void TearDownTestCase() {
    // print dict
    NSLog(@"Performance Data:");
    for (auto const& entry : perftestresults) {
      // NSLog(@"%i%i%i : %f", std::get<0>(entry.first),
      // std::get<1>(entry.first), std::get<2>(entry.first), entry.second);
      NSLog(@"Quic %i    Html2 %i    AKD4 %i    Cronet %i: %f",
            std::get<0>(entry.first), std::get<1>(entry.first),
            std::get<2>(entry.first), std::get<3>(entry.first), entry.second);
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

    // Cronet(netlog+quic):             Elapsed Average:   16738.7ms   Max:
    // 17617.1ms Cronet(nonetlog+quic):           Elapsed Average:    1976.75ms
    // Max:  3253.72ms Cronet(nonetlog+noquic):         Elapsed Average:
    // 829.827ms Max:  1221.86ms Cronet(nonetlog+quic+nohttp2):   Elapsed
    // Average:    1947.5ms   Max:  2291.02ms Cronet(nonetlog+noquic+nohttp2):
    // Elapsed Average:     848.385ms Max:  1321.86ms

    // Platform(netlog+?quic):   Elapsed Average:    1177.87ms  Max:  1505.52ms
    // Platform(nonetlog+?quic): Elapsed Average:1006.13ms Max:1779.11ms

    //[Cronet setRequestFilterBlock:^(NSURLRequest* request) {
    //  return YES; // 96634 ms
    //  //return NO; // 96404 ms
    //}];

    // StartCronet
    [Cronet setUserAgent:@"CronetTest/1.0.0.0" partial:NO];
    [Cronet setQuicEnabled:std::get<0>(GetParam())];
    [Cronet setHttp2Enabled:std::get<1>(GetParam())];
    [Cronet setAcceptLanguages:@"en-US,en"];
    //[Cronet addQuicHint:@"test.example.com" port:443 altPort:443];
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
    // StartedCronet

    //[Cronet startNetLogToFile:@"netlog7.json" logBytes:NO];
    [Cronet registerHttpProtocolHandler];

    NSURLSessionConfiguration* config =
        [NSURLSessionConfiguration ephemeralSessionConfiguration];
    [Cronet installIntoSessionConfiguration:config];
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
// NSMutableDictionary *PerfTest::perftestresults = [NSMutableDictionary
// dictionary];
std::map<testconfig_t, double> PerfTest::perftestresults;

TEST_P(PerfTest, NSURLSessionReceivesImageLoop) {
  int iterations = 50;
  long size = 19307439;
  LOG(INFO) << "Downloading " << size << " bytes " << iterations << " times.";
  // NSTimeInterval elapsed_avg = 0;
  // NSTimeInterval elapsed_max = 0;

  NSURL* url = net::NSURLWithGURL(
      GURL("https://www.gstatic.com/chat/hangouts/bg/davec.jpg"));

  [delegate_ reset];
  NSDate* start = [NSDate date];
  for (int i = 0; i < iterations; ++i) {
    //__block BOOL block_used = NO;
    NSURLSessionDataTask* task = [session_ dataTaskWithURL:url];
    [Cronet setRequestFilterBlock:^(NSURLRequest* request) {
      // block_used = YES;
      EXPECT_EQ([request URL], url);
      // return std::get<3>(GetParam());
      // return YES;
      return NO;
    }];

    StartDataTaskAndWaitForCompletion(task);
    //[task resume];
    //[delegate_ waitForDone];
  }

  // for (int i = 0; i < iterations; ++i) {
  //  [delegate_ waitForDone];
  //}

  NSTimeInterval elapsed = -[start timeIntervalSinceNow];
  // elapsed_avg += elapsed;
  // if (elapsed > elapsed_max)
  //   elapsed_max = elapsed;
  // //EXPECT_TRUE(block_used);
  EXPECT_EQ(nil, [delegate_ error]);
  // EXPECT_EQ(size, [delegate_ totalBytesReceived]);

  // Release the response buffer.
  LOG(INFO) << "Elapsed Total:" << elapsed * 1000 << "ms";

  // perftestresults[GetParam()] = elapsed * 1000;
  //[perftestresults setObject:[NSString stringWithFormat:@"%f", elapsed]
  // forKey:
  //    [NSString stringWithFormat:@"%i", std::get<0>(GetParam())]];
  perftestresults[GetParam()] = elapsed;

  // LOG(INFO) << "Elapsed Average:" << elapsed_avg * 1000 / iterations
  //          << "ms Max:" << elapsed_max * 1000 << "ms";
}

INSTANTIATE_TEST_CASE_P(Loops, PerfTest, generator);
// INSTANTIATE_TEST_CASE_P(Loops, PerfTest, ::testing::Values((std::tuple<BOOL>)
// {NO}, (std::tuple<BOOL>) {YES}));

TEST_F(PerfTest, NSURLSessionReceivesBigHttpDataLoop) {
  int iterations = 50;
  long size = 10 * 1024 * 1024;
  LOG(INFO) << "Downloading " << size << " bytes " << iterations << " times.";
  // NSTimeInterval elapsed_avg = 0;
  // NSTimeInterval elapsed_max = 0;
  NSURL* url = net::NSURLWithGURL(GURL(TestServer::PrepareBigDataURL(size)));

  [delegate_ reset];
  NSDate* start = [NSDate date];
  for (int i = 0; i < iterations; ++i) {
    //__block BOOL block_used = NO;
    NSURLSessionDataTask* task = [session_ dataTaskWithURL:url];
    //[Cronet setRequestFilterBlock:^(NSURLRequest* request) {
    //  block_used = YES;
    //  EXPECT_EQ([request URL], url);
    //  return YES;
    //}];

    // StartDataTaskAndWaitForCompletion(task);
    [task resume];

    [delegate_ waitForDone];
  }

  // for (int i = 0; i < iterations; ++i) {
  //  [delegate_ waitForDone];
  //}

  NSTimeInterval elapsed = -[start timeIntervalSinceNow];
  // elapsed_avg += elapsed;
  // if (elapsed > elapsed_max)
  //   elapsed_max = elapsed;
  // //EXPECT_TRUE(block_used);
  EXPECT_EQ(nil, [delegate_ error]);
  // EXPECT_EQ(size, [delegate_ totalBytesReceived]);

  // Release the response buffer.
  TestServer::ReleaseBigDataURL();
  LOG(INFO) << "Elapsed Total:" << elapsed * 1000 << "ms";
  // LOG(INFO) << "Elapsed Average:" << elapsed_avg * 1000 / iterations
  //          << "ms Max:" << elapsed_max * 1000 << "ms";
}

TEST_F(PerfTestF, NSURLSessionReceivesBigImageLoop) {
  int iterations = 20;
  // long size = 10 * 1024 * 1024;
  long size = 19307439;
  LOG(INFO) << "Downloading " << size << " bytes " << iterations << " times.";
  NSTimeInterval elapsed_avg = 0;
  NSTimeInterval elapsed_max = 0;

  // NSURL* url =
  // //net::NSURLWithGURL(GURL(TestServer::PrepareBigDataURL(size)));
  NSURL* url = net::NSURLWithGURL(
      GURL("https://www.gstatic.com/chat/hangouts/bg/davec.jpg"));

  for (int i = 0; i < iterations; ++i) {
    [delegate_ reset];
    //__block BOOL block_used = NO;
    NSURLSessionDataTask* task = [session_ dataTaskWithURL:url];
    [Cronet setRequestFilterBlock:^(NSURLRequest* request) {
      // block_used = YES;
      EXPECT_EQ([request URL], url);
      return YES;
    }];
    NSDate* start = [NSDate date];

    StartDataTaskAndWaitForCompletion(task);

    NSTimeInterval elapsed = -[start timeIntervalSinceNow];
    elapsed_avg += elapsed;
    if (elapsed > elapsed_max)
      elapsed_max = elapsed;
    // EXPECT_TRUE(block_used);
    EXPECT_EQ(nil, [delegate_ error]);
    EXPECT_EQ(size, [delegate_ totalBytesReceived]);
  }
  // Release the response buffer.
  // TestServer::ReleaseBigDataURL();
  LOG(INFO) << "Elapsed Average:" << elapsed_avg * 1000 / iterations
            << "ms Max:" << elapsed_max * 1000 << "ms";
  LOG(INFO) << "Elapsed Total:" << elapsed_avg * 1000 << "ms";
}

TEST_F(PerfTestF, NSURLSessionReceivesBigImageLoopParallel) {
  int iterations = 20;
  // long size = 10 * 1024 * 1024;
  long size = 19307439;
  LOG(INFO) << "Downloading " << size << " bytes " << iterations << " times.";
  // NSTimeInterval elapsed_avg = 0;
  // NSTimeInterval elapsed_max = 0;

  // NSURL* url =
  // //net::NSURLWithGURL(GURL(TestServer::PrepareBigDataURL(size)));
  NSURL* url = net::NSURLWithGURL(
      GURL("https://www.gstatic.com/chat/hangouts/bg/davec.jpg"));

  [delegate_ reset];
  NSDate* start = [NSDate date];
  for (int i = 0; i < iterations; ++i) {
    //__block BOOL block_used = NO;
    NSURLSessionDataTask* task = [session_ dataTaskWithURL:url];
    [Cronet setRequestFilterBlock:^(NSURLRequest* request) {
      // block_used = YES;
      EXPECT_EQ([request URL], url);
      return YES;
    }];

    // StartDataTaskAndWaitForCompletion(task);
    [task resume];
  }

  for (int i = 0; i < iterations; ++i) {
    [delegate_ waitForDone];
    NSLog(@"fin: %i", i);
  }

  NSTimeInterval elapsed = -[start timeIntervalSinceNow];
  // elapsed_avg += elapsed;
  // if (elapsed > elapsed_max)
  //  elapsed_max = elapsed;
  ////EXPECT_TRUE(block_used);
  NSLog(@"NSError: %@", [delegate_ error]);
  EXPECT_EQ(nil, [delegate_ error]);
  // EXPECT_EQ(size, [delegate_ totalBytesReceived]);

  // Release the response buffer.
  // TestServer::ReleaseBigDataURL();
  LOG(INFO) << "Elapsed Total:" << elapsed * 1000 << "ms";
  // LOG(INFO) << "Elapsed Average:" << elapsed_avg * 1000 / iterations
  //          << "ms Max:" << elapsed_max * 1000 << "ms";
}
}

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cronet/Cronet.h>

#include "base/strings/sys_string_conversions.h"
#include "components/cronet/ios/cronet_metrics.h"
#include "components/cronet/ios/test/cronet_test_base.h"
#include "components/cronet/ios/test/start_cronet.h"
#include "components/cronet/ios/test/test_server.h"
#include "components/grpc_support/test/quic_test_server.h"
#include "ios/net/crn_http_protocol_handler.h"
#include "net/base/mac/url_conversions.h"
#include "testing/gtest_mac.h"
#include "url/gurl.h"

@interface TestMetricsDelegate : NSObject<CronetMetricsDelegate>
@property(readonly) NSURLSessionTaskMetrics* taskMetrics NS_AVAILABLE_IOS(10.0);
@end

@implementation TestMetricsDelegate

@synthesize taskMetrics = _taskMetrics;

- (void)URLSession:(NSURLSession*)session task:(NSURLSessionTask*)task
                    didFinishCollectingMetrics:(NSURLSessionTaskMetrics*)metrics
    NS_AVAILABLE_IOS(10.0) {
  _taskMetrics = metrics;
}

@end

namespace cronet {

class CronetMetricsTest : public CronetTestBase {
 protected:
  CronetMetricsTest() {}
  ~CronetMetricsTest() override {}

  void SetUp() override {
    CronetTestBase::SetUp();
    TestServer::Start();

    [Cronet setBrotliEnabled:YES];
    StartCronet(grpc_support::GetQuicTestServerPort());

    [Cronet registerHttpProtocolHandler];
    NSURLSessionConfiguration* config =
        [NSURLSessionConfiguration ephemeralSessionConfiguration];
    config.requestCachePolicy = NSURLRequestReloadIgnoringLocalCacheData;
    [Cronet installIntoSessionConfiguration:config];
    session_ = [NSURLSession sessionWithConfiguration:config
                                             delegate:delegate_
                                        delegateQueue:nil];
  }

  void TearDown() override {
    [Cronet shutdownForTesting];

    TestServer::Shutdown();
    CronetTestBase::TearDown();
  }

  NSURLSession* session_;
};

TEST_F(CronetMetricsTest, Delegate) {
  if (@available(iOS 10, *)) {
    TestMetricsDelegate* metrics_delegate =
        [[TestMetricsDelegate alloc] init];
    EXPECT_TRUE([Cronet addMetricsDelegate:metrics_delegate]);
    EXPECT_FALSE([Cronet addMetricsDelegate:metrics_delegate]);
    EXPECT_TRUE([Cronet removeMetricsDelegate:metrics_delegate]);
    EXPECT_FALSE([Cronet removeMetricsDelegate:metrics_delegate]);
  }
}

TEST_F(CronetMetricsTest, Setters) {
  if (@available(iOS 10, *)) {
    CronetTransactionMetrics* cronet_metrics =
        [[CronetTransactionMetrics alloc] init];

    NSDate* test_date = [NSDate date];
    NSURLRequest* test_req = [NSURLRequest
        requestWithURL:[NSURL URLWithString:@"test.example.com"]];
    NSURLResponse* test_resp = [[NSURLResponse alloc]
                  initWithURL:[NSURL URLWithString:@"test.example.com"]
                     MIMEType:@"text/plain"
        expectedContentLength:128
             textEncodingName:@"ascii"];

    [cronet_metrics setRequest:test_req];
    [cronet_metrics setResponse:test_resp];

    [cronet_metrics setFetchStartDate:test_date];
    [cronet_metrics setDomainLookupStartDate:test_date];
    [cronet_metrics setDomainLookupEndDate:test_date];
    [cronet_metrics setConnectStartDate:test_date];
    [cronet_metrics setSecureConnectionStartDate:test_date];
    [cronet_metrics setSecureConnectionEndDate:test_date];
    [cronet_metrics setConnectEndDate:test_date];
    [cronet_metrics setRequestStartDate:test_date];
    [cronet_metrics setRequestEndDate:test_date];
    [cronet_metrics setResponseStartDate:test_date];
    [cronet_metrics setResponseEndDate:test_date];

    [cronet_metrics setNetworkProtocolName:@"http/2"];
    [cronet_metrics setProxyConnection:YES];
    [cronet_metrics setReusedConnection:YES];
    [cronet_metrics setResourceFetchType:
                        NSURLSessionTaskMetricsResourceFetchTypeNetworkLoad];

    NSURLSessionTaskTransactionMetrics* metrics = cronet_metrics;

    EXPECT_EQ([metrics request], test_req);
    EXPECT_EQ([metrics response], test_resp);

    EXPECT_EQ([metrics fetchStartDate], test_date);
    EXPECT_EQ([metrics domainLookupStartDate], test_date);
    EXPECT_EQ([metrics domainLookupEndDate], test_date);
    EXPECT_EQ([metrics connectStartDate], test_date);
    EXPECT_EQ([metrics secureConnectionStartDate], test_date);
    EXPECT_EQ([metrics secureConnectionEndDate], test_date);
    EXPECT_EQ([metrics connectEndDate], test_date);
    EXPECT_EQ([metrics requestStartDate], test_date);
    EXPECT_EQ([metrics requestEndDate], test_date);
    EXPECT_EQ([metrics responseStartDate], test_date);
    EXPECT_EQ([metrics responseEndDate], test_date);

    EXPECT_EQ([metrics networkProtocolName], @"http/2");
    EXPECT_EQ([metrics isProxyConnection], YES);
    EXPECT_EQ([metrics isReusedConnection], YES);
    EXPECT_EQ([metrics resourceFetchType],
              NSURLSessionTaskMetricsResourceFetchTypeNetworkLoad);
  }
}

TEST_F(CronetMetricsTest, CallbackStyle) {
  if (@available(iOS 10, *)) {
    TestMetricsDelegate* metrics_delegate = [[TestMetricsDelegate alloc] init];
    [Cronet addMetricsDelegate:metrics_delegate];

    NSURL* url = net::NSURLWithGURL(GURL(grpc_support::kTestServerSimpleUrl));
    __block BOOL block_used = NO;
    NSURLSessionDataTask* task = [session_ dataTaskWithURL:url];
    [Cronet setRequestFilterBlock:^(NSURLRequest* request) {
      block_used = YES;
      EXPECT_EQ([request URL], url);
      return YES;
    }];
    StartDataTaskAndWaitForCompletion(task);
    EXPECT_TRUE(block_used);
    EXPECT_EQ(nil, [delegate_ error]);
    EXPECT_STREQ(grpc_support::kSimpleBodyValue,
                 base::SysNSStringToUTF8([delegate_ responseBody]).c_str());

    NSLog(@"protocol: %@", metrics_delegate.taskMetrics.transactionMetrics[0].networkProtocolName);

    EXPECT_TRUE([metrics_delegate taskMetrics]);
    EXPECT_TRUE([metrics_delegate.taskMetrics.transactionMetrics[0].networkProtocolName containsString:@"quic"]);
  }
}

TEST_F(CronetMetricsTest, ProtocolIsQuic) {
  if (@available(iOS 10, *)) {
    NSURL* url = net::NSURLWithGURL(GURL(grpc_support::kTestServerSimpleUrl));

    NSLog(@"URL: %@", url);

    __block BOOL block_used = NO;
    NSURLSessionDataTask* task = [session_ dataTaskWithURL:url];
    [Cronet setRequestFilterBlock:^(NSURLRequest* request) {
      block_used = YES;
      EXPECT_EQ([request URL], url);
      return YES;
    }];
    StartDataTaskAndWaitForCompletion(task);
    EXPECT_TRUE(block_used);
    EXPECT_EQ(nil, [delegate_ error]);
    EXPECT_STREQ(grpc_support::kSimpleBodyValue,
                 base::SysNSStringToUTF8([delegate_ responseBody]).c_str());

    NSURLSessionTaskMetrics* task_metrics = [delegate_ taskMetrics];
    ASSERT_TRUE(task_metrics);
    ASSERT_EQ(1lU, task_metrics.transactionMetrics.count);
    NSURLSessionTaskTransactionMetrics* metrics =
        task_metrics.transactionMetrics.firstObject;
    ASSERT_TRUE([metrics isMemberOfClass:[CronetTransactionMetrics class]]);

    EXPECT_TRUE([metrics.networkProtocolName containsString:@"quic"]);
  }
}

TEST_F(CronetMetricsTest, ProtocolIsNotQuic) {
  if (@available(iOS 10, *)) {
    NSURL* url = net::NSURLWithGURL(GURL(TestServer::GetSimpleURL()));

    __block BOOL block_used = NO;
    NSURLSessionDataTask* task = [session_ dataTaskWithURL:url];
    [Cronet setRequestFilterBlock:^(NSURLRequest* request) {
      block_used = YES;
      EXPECT_EQ([request URL], url);
      return YES;
    }];
    StartDataTaskAndWaitForCompletion(task);
    EXPECT_TRUE(block_used);
    EXPECT_EQ(nil, [delegate_ error]);
    EXPECT_STREQ("The quick brown fox jumps over the lazy dog.",
                 base::SysNSStringToUTF8([delegate_ responseBody]).c_str());

    NSURLSessionTaskMetrics* task_metrics = [delegate_ taskMetrics];
    ASSERT_TRUE(task_metrics);
    ASSERT_EQ(1lU, task_metrics.transactionMetrics.count);
    NSURLSessionTaskTransactionMetrics* metrics =
        task_metrics.transactionMetrics.firstObject;
    //ASSERT_TRUE([metrics isMemberOfClass:[CronetTransactionMetrics class]]);

    // When using platform, this is "http/1.1"; but here we print "unknown".
    // Why?
    NSLog(@"protocol: %@", metrics.networkProtocolName);
    EXPECT_FALSE([metrics.networkProtocolName containsString:@"quic"]);
  }
}

}  // namespace cronet

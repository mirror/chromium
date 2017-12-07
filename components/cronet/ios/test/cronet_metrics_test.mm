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
#import "ios/net/crn_http_protocol_handler.h"
#import "net/base/mac/url_conversions.h"
#include "testing/gtest_mac.h"
#include "url/gurl.h"

namespace cronet {

class CronetMetricsTest : public CronetTestBase {
 protected:
  CronetMetricsTest() {}
  ~CronetMetricsTest() override {}

  void SetUp() override {
    CronetTestBase::SetUp();
    TestServer::Start();

    [Cronet setMetricsEnabled:YES];
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
    ASSERT_FALSE([Cronet metricsTaskMapSizeForTesting]);

    [Cronet shutdownForTesting];

    TestServer::Shutdown();
    CronetTestBase::TearDown();
  }

  NSURLSession* session_;
};

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

    // Confirm that metrics data is the correct type.
    ASSERT_TRUE([metrics.fetchStartDate isKindOfClass:[NSDate class]]);
    ASSERT_TRUE([metrics.domainLookupStartDate isKindOfClass:[NSDate class]]);
    ASSERT_TRUE([metrics.domainLookupEndDate isKindOfClass:[NSDate class]]);
    ASSERT_TRUE([metrics.connectStartDate isKindOfClass:[NSDate class]]);
    ASSERT_TRUE(
        [metrics.secureConnectionStartDate isKindOfClass:[NSDate class]]);
    ASSERT_TRUE([metrics.secureConnectionEndDate isKindOfClass:[NSDate class]]);
    ASSERT_TRUE([metrics.connectEndDate isKindOfClass:[NSDate class]]);
    ASSERT_TRUE([metrics.requestStartDate isKindOfClass:[NSDate class]]);
    ASSERT_TRUE([metrics.requestEndDate isKindOfClass:[NSDate class]]);
    ASSERT_TRUE([metrics.responseStartDate isKindOfClass:[NSDate class]]);
    ASSERT_TRUE([metrics.responseEndDate isKindOfClass:[NSDate class]]);
    ASSERT_TRUE([metrics.networkProtocolName isKindOfClass:[NSString class]]);

    // Confirm that the metrics values are sane.
    ASSERT_NE(
        [[metrics domainLookupStartDate] compare:[metrics domainLookupEndDate]],
        NSOrderedDescending);
    ASSERT_NE([[metrics connectStartDate] compare:[metrics connectEndDate]],
              NSOrderedDescending);
    ASSERT_NE([[metrics secureConnectionStartDate]
                  compare:[metrics secureConnectionEndDate]],
              NSOrderedDescending);
    ASSERT_NE([[metrics requestStartDate] compare:[metrics requestEndDate]],
              NSOrderedDescending);
    ASSERT_NE([[metrics responseStartDate] compare:[metrics responseEndDate]],
              NSOrderedDescending);

    ASSERT_FALSE([metrics isProxyConnection]);

    NSLog(@"protocol: %@", metrics.networkProtocolName);
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
    ASSERT_TRUE([metrics isMemberOfClass:[CronetTransactionMetrics class]]);

    EXPECT_FALSE([metrics.networkProtocolName containsString:@"quic"]);
  }
}

TEST_F(CronetMetricsTest, PlatformComparison) {
  if (@available(iOS 10, *)) {
    NSURL* url = net::NSURLWithGURL(GURL(TestServer::GetSimpleURL()));

    // Perform a connection using Cronet.

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

    NSURLSessionTaskMetrics* cronet_task_metrics = [delegate_ taskMetrics];
    ASSERT_TRUE(cronet_task_metrics);
    ASSERT_EQ(1lU, cronet_task_metrics.transactionMetrics.count);
    NSURLSessionTaskTransactionMetrics* cronet_metrics =
        cronet_task_metrics.transactionMetrics.firstObject;

    // Perform a connection using the platform stack.

    block_used = NO;
    task = [session_ dataTaskWithURL:url];
    [Cronet setRequestFilterBlock:^(NSURLRequest* request) {
      block_used = YES;
      EXPECT_EQ([request URL], url);
      return NO;
    }];
    StartDataTaskAndWaitForCompletion(task);
    EXPECT_TRUE(block_used);
    EXPECT_EQ(nil, [delegate_ error]);
    EXPECT_STREQ("The quick brown fox jumps over the lazy dog.",
                 base::SysNSStringToUTF8([delegate_ responseBody]).c_str());

    NSURLSessionTaskMetrics* platform_task_metrics = [delegate_ taskMetrics];
    ASSERT_TRUE(platform_task_metrics);
    ASSERT_EQ(1lU, platform_task_metrics.transactionMetrics.count);
    NSURLSessionTaskTransactionMetrics* platform_metrics =
        platform_task_metrics.transactionMetrics.firstObject;

    // Compare platform and Cronet metrics data.

    ASSERT_TRUE([cronet_metrics.networkProtocolName
        isEqualToString:platform_metrics.networkProtocolName]);
  }
}

TEST_F(CronetMetricsTest, InvalidURL) {
  if (@available(iOS 10, *)) {
    NSURL* url = net::NSURLWithGURL(GURL("http://notfound.example.com"));

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
    EXPECT_TRUE([delegate_ error]);

    NSURLSessionTaskMetrics* task_metrics = [delegate_ taskMetrics];
    ASSERT_TRUE(task_metrics);
    ASSERT_EQ(1lU, task_metrics.transactionMetrics.count);
    NSURLSessionTaskTransactionMetrics* metrics =
        task_metrics.transactionMetrics.firstObject;
    ASSERT_TRUE([metrics isMemberOfClass:[CronetTransactionMetrics class]]);

    ASSERT_TRUE([metrics fetchStartDate]);
    ASSERT_FALSE([metrics domainLookupStartDate]);
    ASSERT_FALSE([metrics domainLookupEndDate]);
    ASSERT_FALSE([metrics connectStartDate]);
    ASSERT_FALSE([metrics secureConnectionStartDate]);
    ASSERT_FALSE([metrics secureConnectionEndDate]);
    ASSERT_FALSE([metrics connectEndDate]);
    ASSERT_FALSE([metrics requestStartDate]);
    ASSERT_FALSE([metrics requestEndDate]);
    ASSERT_FALSE([metrics responseStartDate]);
  }
}

TEST_F(CronetMetricsTest, CanceledRequest) {
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

    StartDataTaskAndWaitForCompletion(task, 1);
    [task cancel];

    EXPECT_TRUE(block_used);
    EXPECT_NE(nil, [delegate_ error]);
    NSLog(@"error: %@", [delegate_ error]);
  }
}

TEST_F(CronetMetricsTest, ReusedConnection) {
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

    // Second connection

    block_used = NO;
    task = [session_ dataTaskWithURL:url];
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

    task_metrics = [delegate_ taskMetrics];
    ASSERT_TRUE(task_metrics);
    ASSERT_EQ(1lU, task_metrics.transactionMetrics.count);
    metrics = task_metrics.transactionMetrics.firstObject;

    ASSERT_TRUE([metrics isReusedConnection]);
    ASSERT_FALSE([metrics domainLookupStartDate]);
    ASSERT_FALSE([metrics domainLookupEndDate]);
    ASSERT_FALSE([metrics connectStartDate]);
    ASSERT_FALSE([metrics secureConnectionStartDate]);
    ASSERT_FALSE([metrics secureConnectionEndDate]);
    ASSERT_FALSE([metrics connectEndDate]);
  }
}

}  // namespace cronet

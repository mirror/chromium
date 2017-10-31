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

FOUNDATION_EXPORT GRPC_SUPPORT_EXPORT NS_AVAILABLE_IOS(10.0)
@interface CronetMetrics : NSURLSessionTaskTransactionMetrics

@property(copy, readwrite) NSURLRequest* request;
@property(copy, readwrite) NSURLResponse* response;

@property(copy, readwrite) NSDate* fetchStartDate;
@property(copy, readwrite) NSDate* domainLookupStartDate;
@property(copy, readwrite) NSDate* domainLookupEndDate;
@property(copy, readwrite) NSDate* connectStartDate;
@property(copy, readwrite) NSDate* secureConnectionStartDate;
@property(copy, readwrite) NSDate* secureConnectionEndDate;
@property(copy, readwrite) NSDate* connectEndDate;
@property(copy, readwrite) NSDate* requestStartDate;
@property(copy, readwrite) NSDate* requestEndDate;
@property(copy, readwrite) NSDate* responseStartDate;
@property(copy, readwrite) NSDate* responseEndDate;

@property(copy, readwrite) NSString* networkProtocolName;
@property(assign, readwrite, getter=isProxyConnection) BOOL proxyConnection;
@property(assign, readwrite, getter=isReusedConnection) BOOL reusedConnection;
@property(assign, readwrite)
    NSURLSessionTaskMetricsResourceFetchType resourceFetchType;

@end

namespace cronet {

TEST(MetricsTest, Setters) {
  // can this be made cleaner?
  if (@available(iOS 10, *)) {
    CronetMetrics* cronet_metrics = [[CronetMetrics alloc] init];

    // uninitialized dummy objects soley for testing set/get functionality
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

}  // namespace cronet

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_IOS_CRONET_METRICS_H_
#define COMPONENTS_CRONET_IOS_CRONET_METRICS_H_

#import <Foundation/Foundation.h>

#include "components/grpc_support/include/bidirectional_stream_c.h"
#include "ios/net/crn_http_protocol_handler.h"
#include "net/http/http_network_session.h"

// These are internal versions of NSURLSessionTaskTransactionMetrics and
// NSURLSessionTaskMetrics, defined primarily so that Cronet can
// initialize them and set their properties (the iOS classes are readonly)

// The correspondences are
//   CronetTransactionMetrics -> NSURLSessionTaskTransactionMetrics
//   CronetMetrics -> NSURLSessionTaskMetrics

FOUNDATION_EXPORT GRPC_SUPPORT_EXPORT NS_AVAILABLE_IOS(10.0)
@interface CronetTransactionMetrics : NSURLSessionTaskTransactionMetrics

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

NS_AVAILABLE_IOS(10.0) @interface CronetMetrics : NSURLSessionTaskMetrics
@property(copy, readwrite)
    NSArray<NSURLSessionTaskTransactionMetrics*>* transactionMetrics;
@end

// Helper methods.

// Helper method that converts the ticks data found in LoadTimingInfo to an
// NSDate value to be used in client-side data.
NSDate*
    ticksToDate(const net::LoadTimingInfo& reference, base::TimeTicks ticks);

// Copy of GetProxy in components/cronet/android/cronet_url_request_adapter.cc
// Returns the string representation of the HostPortPair of the proxy server
// that was used to fetch the response.
std::string getProxy(const net::HttpResponseInfo& response_info);

// Converts net::Metrics metrics data into CronetTransactionMetrics (which
// importantly implements the NSURLSessionTaskTransactionMetrics API)
CronetTransactionMetrics* nativeToIOSMetrics(net::Metrics metrics)
    NS_AVAILABLE_IOS(10.0);

#endif // COMPONENTS_CRONET_IOS_CRONET_METRICS_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_IOS_CRONET_METRICS_H_
#define COMPONENTS_CRONET_IOS_CRONET_METRICS_H_

#import <Foundation/Foundation.h>

#include "components/grpc_support/include/bidirectional_stream_c.h"
#import "ios/net/crn_http_protocol_handler.h"
#include "net/http/http_network_session.h"

// These are internal versions of NSURLSessionTaskTransactionMetrics and
// NSURLSessionTaskMetrics, defined primarily so that Cronet can
// initialize them and set their properties (the iOS classes are readonly).

// The correspondences are
//   CronetTransactionMetrics -> NSURLSessionTaskTransactionMetrics
//   CronetMetrics -> NSURLSessionTaskMetrics

FOUNDATION_EXPORT GRPC_SUPPORT_EXPORT NS_AVAILABLE_IOS(10.0)
@interface CronetTransactionMetrics : NSURLSessionTaskTransactionMetrics

// All of the below redefined as readwrite.

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
// Redefined as readwrite.
@property(copy, readwrite)
    NSArray<NSURLSessionTaskTransactionMetrics*>* transactionMetrics;
@end

class CronetMetricsDelegate : public net::MetricsDelegate {
 public:
  CronetMetricsDelegate() {}
  void OnStopNetRequest(std::unique_ptr<net::Metrics> metrics) override;

  // Returns the metrics collected for a specific task (removing that task's
  // entry from the map in the process).
  // It is called exactly once by the swizzled delegate proxy (see below),
  // uses it to retrieve metrics data collected by net/ and pass them on to the
  // client.
  // If there is no metrics data for the pass task, this returns nullptr.
  static std::unique_ptr<net::Metrics> MetricsForTask(NSURLSessionTask* task);

 private:
  static std::map<NSURLSessionTask*, std::unique_ptr<net::Metrics>>
      task_metrics_map_ NS_AVAILABLE_IOS(10.0);
};

@interface NSURLSession (Cronet)

+ (void)swizzleSessionWithConfiguration;

@end

#endif // COMPONENTS_CRONET_IOS_CRONET_METRICS_H_

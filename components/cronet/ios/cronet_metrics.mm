// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "components/cronet/ios/cronet_metrics.h"

#import <objc/runtime.h>
#include "base/strings/sys_string_conversions.h"

@implementation CronetTransactionMetrics

@synthesize request = _request;
@synthesize response = _response;

@synthesize fetchStartDate = _fetchStartDate;
@synthesize domainLookupStartDate = _domainLookupStartDate;
@synthesize domainLookupEndDate = _domainLookupEndDate;
@synthesize connectStartDate = _connectStartDate;
@synthesize secureConnectionStartDate = _secureConnectionStartDate;
@synthesize secureConnectionEndDate = _secureConnectionEndDate;
@synthesize connectEndDate = _connectEndDate;
@synthesize requestStartDate = _requestStartDate;
@synthesize requestEndDate = _requestEndDate;
@synthesize responseStartDate = _responseStartDate;
@synthesize responseEndDate = _responseEndDate;

@synthesize networkProtocolName = _networkProtocolName;
@synthesize proxyConnection = _proxyConnection;
@synthesize reusedConnection = _reusedConnection;
@synthesize resourceFetchType = _resourceFetchType;

- (instancetype)init {
  // NSURLSessionTaskTransactionMetrics class is not supposed to be extended.
  // Its default init method initialized an internal class, and therefore
  // needs to be overridden to explicitly initialize (and return) an instance
  // of this class.
  // The |self = old_self| swap is necessary because [super init] must be
  // assigned to self (or returned immediately), but in this case is returning
  // a value of the wrong type.

  id old_self = self;
  self = [super init];
  self = old_self;
  return old_self;
}

@end

@implementation CronetMetrics

@synthesize transactionMetrics = _transactionMetrics;

- (instancetype)init {
  id old_self = self;
  self = [super init];
  // NSURLSessionTaskMetrics class is not supposed to be extended.  That is
  // why we replace |self| returned by [super init] by the original value.
  // We still assign it to |self| because compiler requires that the result
  // of [super init] is assigned to |self| or returned immediatly.
  self = old_self;
  return old_self;
}

@end

namespace {

// Helper method that converts the ticks data found in LoadTimingInfo to an
// NSDate value to be used in client-side data.
NSDate* TicksToDate(const net::LoadTimingInfo& reference,
                    const base::TimeTicks& ticks) {
  base::Time ticks_since_1970 =
      (reference.request_start_time + (ticks - reference.request_start));
  return [NSDate dateWithTimeIntervalSince1970:ticks_since_1970.ToDoubleT()];
}

// Converts net::Metrics metrics data into CronetTransactionMetrics (which
// importantly implements the NSURLSessionTaskTransactionMetrics API)
CronetTransactionMetrics* NativeToIOSMetrics(net::Metrics& metrics)
    NS_AVAILABLE_IOS(10.0) {
  NSURLSessionTask* task = metrics.task;
  const net::LoadTimingInfo& load_timing_info = metrics.load_timing_info;
  const net::HttpResponseInfo& response_info = metrics.response_info;

  CronetTransactionMetrics* transaction_metrics =
      [[CronetTransactionMetrics alloc] init];

  // where do these come from?
  [transaction_metrics setRequest:[task currentRequest]];
  [transaction_metrics setResponse:nil];

  [transaction_metrics setFetchStartDate:nil];  // ???

  transaction_metrics.domainLookupStartDate =
      TicksToDate(load_timing_info, load_timing_info.connect_timing.dns_start);
  transaction_metrics.domainLookupEndDate =
      TicksToDate(load_timing_info, load_timing_info.connect_timing.dns_end);

  transaction_metrics.connectStartDate = TicksToDate(
      load_timing_info, load_timing_info.connect_timing.connect_start);
  transaction_metrics.secureConnectionStartDate =
      TicksToDate(load_timing_info, load_timing_info.connect_timing.ssl_start);
  transaction_metrics.secureConnectionEndDate =
      TicksToDate(load_timing_info, load_timing_info.connect_timing.ssl_end);
  transaction_metrics.connectEndDate = TicksToDate(
      load_timing_info, load_timing_info.connect_timing.connect_end);

  transaction_metrics.requestStartDate =
      [NSDate dateWithTimeIntervalSince1970:load_timing_info.request_start_time
                                                .ToDoubleT()];
  transaction_metrics.requestEndDate =
      TicksToDate(load_timing_info, load_timing_info.send_end);
  transaction_metrics.responseStartDate =
      TicksToDate(load_timing_info, load_timing_info.receive_headers_end);
  transaction_metrics.responseEndDate = [NSDate
      dateWithTimeIntervalSince1970:response_info.response_time.ToDoubleT()];

  transaction_metrics.networkProtocolName =
      base::SysUTF8ToNSString(response_info.alpn_negotiated_protocol);
  transaction_metrics.proxyConnection = !response_info.proxy_server.is_direct();
  //[transaction_metrics setReusedConnection:nil]; // ???
  //[transaction_metrics setResourceFetchType:nil]; // ???

  return transaction_metrics;
}

}  // namespace

std::map<NSURLSessionTask*, std::unique_ptr<net::Metrics>>
    CronetMetricsDelegate::task_metrics_map_;

std::unique_ptr<net::Metrics> CronetMetricsDelegate::MetricsForTask(
    NSURLSessionTask* task) {
  auto metrics_search = task_metrics_map_.find(task);
  if (metrics_search == task_metrics_map_.end()) {
    return nullptr;
  }

  std::unique_ptr<net::Metrics> metrics = std::move(metrics_search->second);
  task_metrics_map_.erase(metrics_search);  // Remove the entry to free memory.
  return metrics;
}

void CronetMetricsDelegate::OnStopNetRequest(
    std::unique_ptr<net::Metrics> metrics) {
  if (@available(iOS 10, *)) {
    task_metrics_map_[metrics->task] = std::move(metrics);
  }
}

// In order for Cronet to use the iOS metrics collection API, it needs to
// replace the normal NSURLSession mechanism for calling into the delegate
// (so it can provide metrics from net/, instead of the empty metrics that iOS
// would provide otherwise.

#pragma mark - Swizzle
@interface URLSessionTaskDelegateProxy : NSProxy<NSURLSessionTaskDelegate>
- (instancetype)initWithDelegate:(id<NSURLSessionDelegate>)delegate;
@end

@implementation URLSessionTaskDelegateProxy {
  id<NSURLSessionDelegate> _delegate;
}
- (instancetype)initWithDelegate:(id<NSURLSessionDelegate>)delegate {
  _delegate = delegate;
  return self;
}

- (void)forwardInvocation:(NSInvocation*)invocation {
  [invocation setTarget:_delegate];
  [invocation invoke];
}

- (nullable NSMethodSignature*)methodSignatureForSelector:(SEL)sel {
  return [(id)_delegate methodSignatureForSelector:sel];
}

- (void)URLSession:(NSURLSession*)session
                          task:(NSURLSessionTask*)task
    didFinishCollectingMetrics:(NSURLSessionTaskMetrics*)metrics
    NS_AVAILABLE_IOS(10.0) {
  std::unique_ptr<net::Metrics> netMetrics =
      CronetMetricsDelegate::MetricsForTask(task);

  if ([_delegate respondsToSelector:@selector
                 (URLSession:task:didFinishCollectingMetrics:)]) {
    if (netMetrics) {
      CronetTransactionMetrics* cronetTransactionMetrics =
          NativeToIOSMetrics(*netMetrics);

      CronetMetrics* cronetMetrics = [[CronetMetrics alloc] init];
      [cronetMetrics setTransactionMetrics:@[ cronetTransactionMetrics ]];

      [(id<NSURLSessionTaskDelegate>)_delegate URLSession:session
                                                     task:task
                               didFinishCollectingMetrics:cronetMetrics];
    // If there are no metrics is Cronet's task->metrics map, then Cronet is
    // not handling this request, so just transparently pass iOS's collected
    // metrics.
    } else {
      [(id<NSURLSessionTaskDelegate>)_delegate URLSession:session
                                                     task:task
                               didFinishCollectingMetrics:metrics];
    }
  }
}
@end

@implementation NSURLSession (Cronet)

+ (void)swizzleSessionWithConfiguration {
  Class thisClass = object_getClass(self);

  SEL originalSelector =
      @selector(sessionWithConfiguration:delegate:delegateQueue:);
  SEL swizzledSelector =
      @selector(hookSessionWithConfiguration:delegate:delegateQueue:);

  Method originalMethod = class_getInstanceMethod(thisClass, originalSelector);
  Method swizzledMethod = class_getInstanceMethod(thisClass, swizzledSelector);

  method_exchangeImplementations(originalMethod, swizzledMethod);
}

+ (NSURLSession*)
hookSessionWithConfiguration:(NSURLSessionConfiguration*)configuration
                    delegate:(nullable id<NSURLSessionDelegate>)delegate
               delegateQueue:(nullable NSOperationQueue*)queue {
  URLSessionTaskDelegateProxy* delegate_proxy =
      [[URLSessionTaskDelegateProxy alloc] initWithDelegate:delegate];
  // Because the the method implementations are swapped, this is not a
  // recursive call, and instead just forwards the call to the original
  // sessionWithConfiguration method.
  return [self hookSessionWithConfiguration:configuration
                                   delegate:delegate_proxy
                              delegateQueue:queue];
}

@end

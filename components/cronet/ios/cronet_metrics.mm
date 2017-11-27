// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//#import "components/cronet/ios/Cronet.h"
#import "components/cronet/ios/cronet_metrics.h"
#import <objc/runtime.h>

#include "base/strings/sys_string_conversions.h"
#include "ios/net/crn_http_protocol_handler.h"
#include "net/http/http_network_session.h"

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
  id old_self = self;
  self = [super init];
  // NSURLSessionTaskTransactionMetrics class is not supposed to be extended.
  // That is why we replace |self| returned by [super init] by the original
  // value. We still assign it to |self| because compiler requires that the
  // result of [super init] is assigned to |self| or returned immediatly.
  self = old_self;
  return old_self;
}

@end

@implementation CronetMetrics {
  NSArray<NSURLSessionTaskTransactionMetrics*>* _transactionMetrics;
}

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

NSDate* ticksToDate(const net::LoadTimingInfo& reference,
                    base::TimeTicks ticks) {
  return
      [NSDate dateWithTimeIntervalSince1970:(reference.request_start_time +
                                             (ticks - reference.request_start))
                                                .ToDoubleT()];
}

std::string getProxy(const net::HttpResponseInfo& response_info) {
  if (!response_info.proxy_server.is_valid() ||
      response_info.proxy_server.is_direct()) {
    return net::HostPortPair().ToString();
  }
  return response_info.proxy_server.host_port_pair().ToString();
}

CronetTransactionMetrics* nativeToIOSMetrics(net::Metrics metrics)
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
      ticksToDate(load_timing_info, load_timing_info.connect_timing.dns_start);
  transaction_metrics.domainLookupEndDate =
      ticksToDate(load_timing_info, load_timing_info.connect_timing.dns_end);

  transaction_metrics.connectStartDate = ticksToDate(
      load_timing_info, load_timing_info.connect_timing.connect_start);
  transaction_metrics.secureConnectionStartDate =
      ticksToDate(load_timing_info, load_timing_info.connect_timing.ssl_start);
  transaction_metrics.secureConnectionEndDate =
      ticksToDate(load_timing_info, load_timing_info.connect_timing.ssl_end);
  transaction_metrics.connectEndDate = ticksToDate(
      load_timing_info, load_timing_info.connect_timing.connect_end);

  transaction_metrics.requestStartDate =
      [NSDate dateWithTimeIntervalSince1970:load_timing_info.request_start_time
                                                .ToDoubleT()];
  transaction_metrics.requestEndDate =
      ticksToDate(load_timing_info, load_timing_info.send_end);
  transaction_metrics.responseStartDate =
      ticksToDate(load_timing_info, load_timing_info.receive_headers_end);
  transaction_metrics.responseEndDate = [NSDate
      dateWithTimeIntervalSince1970:response_info.response_time.ToDoubleT()];

  transaction_metrics.networkProtocolName =
      base::SysUTF8ToNSString(response_info.alpn_negotiated_protocol);
  transaction_metrics.proxyConnection =
      base::SysUTF8ToNSString(getProxy(response_info));
  //[transaction_metrics setReusedConnection:nil]; // ???
  //[transaction_metrics setResourceFetchType:nil]; // ???

  return transaction_metrics;
}

#pragma mark - Swizzle
@interface URLSessionTaskDelegateProxy : NSProxy<NSURLSessionTaskDelegate>
- (id)initWithDelegate:(id<NSURLSessionDelegate>)delegate;
@end

@implementation URLSessionTaskDelegateProxy {
  id<NSURLSessionDelegate> _delegate;
}
- (id)initWithDelegate:(id<NSURLSessionDelegate>)delegate {
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

  // if no cronet metrics, just pass ios

  net::Metrics net_metrics = [CRNHTTPProtocolHandler metricsForTask:task];

  // If the task is nil, that means that the metrics object was not retrieved
  // from the task metrics map in net, which indicates that Cronet is not
  // handling this result.
  if (net_metrics.task) {
    CronetTransactionMetrics* cronet_transaction_metrics =
        nativeToIOSMetrics(net_metrics);

    CronetMetrics* cronet_metrics = [[CronetMetrics alloc] init];
    [cronet_metrics setTransactionMetrics:@[ cronet_transaction_metrics ]];

    if ([_delegate conformsToProtocol:@protocol(NSURLSessionTaskDelegate)]) {
      [(id<NSURLSessionTaskDelegate>)_delegate URLSession:session
                                                     task:task
                               didFinishCollectingMetrics:cronet_metrics];
    }
  } else {
    if ([_delegate conformsToProtocol:@protocol(NSURLSessionTaskDelegate)]) {
      [(id<NSURLSessionTaskDelegate>)_delegate URLSession:session
                                                     task:task
                               didFinishCollectingMetrics:metrics];
    }
  }
}
@end

@implementation NSURLSession (Cronet)

+ (void)load {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    [self swizzleSessionWithConfiguration];
  });
}

+ (void)swizzleSessionWithConfiguration {
  Class c = object_getClass(self);

  SEL originalSelector =
      @selector(sessionWithConfiguration:delegate:delegateQueue:);
  SEL swizzledSelector =
      @selector(hookSessionWithConfiguration:delegate:delegateQueue:);

  Method originalMethod = class_getInstanceMethod(c, originalSelector);
  Method swizzledMethod = class_getInstanceMethod(c, swizzledSelector);

  method_exchangeImplementations(originalMethod, swizzledMethod);
}

+ (NSURLSession*)
hookSessionWithConfiguration:(NSURLSessionConfiguration*)configuration
                    delegate:(nullable id<NSURLSessionDelegate>)delegate
               delegateQueue:(nullable NSOperationQueue*)queue {

  URLSessionTaskDelegateProxy* delegate_proxy =
      [[URLSessionTaskDelegateProxy alloc] initWithDelegate:delegate];
  return [self hookSessionWithConfiguration:configuration
                                   delegate:delegate_proxy
                              delegateQueue:queue];
}

@end

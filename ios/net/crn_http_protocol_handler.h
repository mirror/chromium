// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_NET_CRN_HTTP_PROTOCOL_HANDLER_H_
#define IOS_NET_CRN_HTTP_PROTOCOL_HANDLER_H_

#import <Foundation/Foundation.h>

#include "net/base/load_timing_info.h"
#include "net/http/http_response_info.h"

namespace net {
class URLRequestContextGetter;
struct LoadTimingInfo;
class HttpResponseInfo;

class HTTPProtocolHandlerDelegate {
 public:
  // Sets the global instance of the HTTPProtocolHandlerDelegate.
  static void SetInstance(HTTPProtocolHandlerDelegate* delegate);

  virtual ~HTTPProtocolHandlerDelegate() {}

  // Returns true if CRNHTTPProtocolHandler should handle the request.
  // Returns false if the request should be passed down the NSURLProtocol chain.
  virtual bool CanHandleRequest(NSURLRequest* request) = 0;

  // If IsRequestSupported returns true, |request| will be processed, otherwise
  // a NSURLErrorUnsupportedURL error is generated.
  virtual bool IsRequestSupported(NSURLRequest* request) = 0;

  // Returns the request context used for requests that are not associated with
  // a RequestTracker. This includes in particular the requests that are not
  // aware of the network stack. Must not return null.
  virtual URLRequestContextGetter* GetDefaultURLRequestContext() = 0;
};

// Delegate class which supplies a metrics callback that is invoked when a
// net request is stopped.
class MetricsDelegate {
 public:
  // Set the global instance of the MetricsDelegate.
  static void SetInstance(MetricsDelegate* delegate);

  virtual ~MetricsDelegate() {}

  // This is invoked once metrics have been been collected by net.
  // It will need to take a metrics object argument.
  virtual void didFinishCollectingMetrics(
      NSURLSessionTask* task,
      const LoadTimingInfo& load_timing_info,
      const HttpResponseInfo& response_info) = 0;
};

struct Metrics {
  NSURLSessionTask* task;
  const LoadTimingInfo load_timing_info;
  const HttpResponseInfo response_info;

  Metrics operator=(Metrics m) {
    return Metrics{task, load_timing_info, response_info};
  }
};

}  // namespace net

// Custom NSURLProtocol handling HTTP and HTTPS requests.
// The HttpProtocolHandler is registered as a NSURLProtocol in the iOS
// system. This protocol is called for each NSURLRequest. This allows
// handling the requests issued by UIWebView using our own network stack.
@interface CRNHTTPProtocolHandler : NSURLProtocol
+ (net::Metrics)metricsForTask:(NSURLSessionTask*)task NS_AVAILABLE_IOS(10.0);
@end

#endif  // IOS_NET_CRN_HTTP_PROTOCOL_HANDLER_H_

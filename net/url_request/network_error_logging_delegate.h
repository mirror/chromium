// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_URL_REQUEST_NETWORK_ERROR_LOGGING_DELEGATE_H_
#define NET_URL_REQUEST_NETWORK_ERROR_LOGGING_DELEGATE_H_

#include <string>

#include "base/callback.h"
#include "base/time/time.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/net_export.h"
#include "net/socket/next_proto.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace net {

class NET_EXPORT NetworkErrorLoggingDelegate {
 public:
  // The details of a network error that are included in an NEL report.
  //
  // See http://wicg.github.io/network-error-logging/#dfn-network-error-object
  // for details on the semantics of each field.
  struct ErrorDetails {
    ErrorDetails();
    ~ErrorDetails();

    GURL uri;
    GURL referrer;
    IPEndPoint server_ip;
    NextProto protocol;
    int status_code;
    base::TimeDelta elapsed_time;
    Error type;
  };

  // A callback used to obtain the details of a network error. Should be called
  // with a pointer to an ErrorDetails struct, and will fill it in.
  using ErrorDetailsCallback = base::OnceCallback<void(ErrorDetails*)>;

  virtual ~NetworkErrorLoggingDelegate() = 0;

  // Called when the network stack receives an "NEL" header over a secure
  // connection. |url| is the URL from which the header was received; |value| is
  // the value of the header.
  //
  // See http://wicg.github.io/network-error-logging/#x`nel`-header-field for
  // details on the configuration available via the header.
  virtual void OnNELHeader(const url::Origin& url,
                           const std::string& value) = 0;

  // Called when the network stack detects a network error.
  //
  // |origin| is the origin the network stack was trying to talk to when it
  // encountered the error.
  //
  // |error| is the error the network stack encountered.
  //
  // |details_callback| is a callback that will fill in the details of the error
  // if the delegate wants them (based on |origin| and |error|). If called, it
  // must be called synchronously, before OnNetworkError returns.
  virtual void OnNetworkError(const url::Origin& origin,
                              Error error,
                              ErrorDetailsCallback details_callback);
};

}  // namespace net

#endif  // NET_URL_REQUEST_NETWORK_ERROR_LOGGING_DELEGATE_H_

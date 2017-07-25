// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NETWORK_ERROR_LOGGING_NETWORK_ERROR_LOGGING_SERVICE_H_
#define COMPONENTS_NETWORK_ERROR_LOGGING_NETWORK_ERROR_LOGGING_SERVICE_H_

#include <string>

#include "base/macros.h"
#include "components/network_error_logging/network_error_logging_export.h"
#include "net/base/net_errors.h"
#include "net/url_request/network_error_logging_delegate.h"

namespace net {
class ReportingService;
}  // namespace net

namespace url {
class Origin;
}  // namespace url

namespace network_error_logging {

class NETWORK_ERROR_LOGGING_EXPORT NetworkErrorLoggingService
    : public net::NetworkErrorLoggingDelegate {
 public:
  // |reporting_service| must outlive this service.
  NetworkErrorLoggingService(net::ReportingService* reporting_service);

  // net::NetworkErrorLoggingDelegate implementation:

  ~NetworkErrorLoggingService() override;

  void OnHeader(const url::Origin& origin, const std::string& value) override;

  void OnNetworkError(const url::Origin& origin,
                      net::Error error,
                      ErrorDetailsCallback details_callback) override;

 private:
  // Unowned.
  net::ReportingService* reporting_service_;

  DISALLOW_COPY_AND_ASSIGN(NetworkErrorLoggingService);
};

}  // namespace network_error_logging

#endif  // COMPONENTS_NETWORK_ERROR_LOGGING_NETWORK_ERROR_LOGGING_SERVICE_H_

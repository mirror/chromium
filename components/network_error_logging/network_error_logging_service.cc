// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/network_error_logging/network_error_logging_service.h"

#include <string>

#include "base/logging.h"
#include "net/base/net_errors.h"
#include "net/reporting/reporting_service.h"
#include "url/origin.h"

namespace network_error_logging {

NetworkErrorLoggingService::NetworkErrorLoggingService(
    net::ReportingService* reporting_service)
    : reporting_service_(reporting_service) {
  DCHECK(reporting_service_);
}

NetworkErrorLoggingService::~NetworkErrorLoggingService() {}

void NetworkErrorLoggingService::OnHeader(const url::Origin& origin,
                                          const std::string& value) {}

void NetworkErrorLoggingService::OnNetworkError(
    const url::Origin& origin,
    net::Error error,
    ErrorDetailsCallback details_callback) {}

}  // namespace network_error_logging

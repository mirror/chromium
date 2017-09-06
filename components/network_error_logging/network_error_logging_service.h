// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NETWORK_ERROR_LOGGING_NETWORK_ERROR_LOGGING_SERVICE_H_
#define COMPONENTS_NETWORK_ERROR_LOGGING_NETWORK_ERROR_LOGGING_SERVICE_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/feature_list.h"
#include "base/macros.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "components/network_error_logging/network_error_logging_export.h"
#include "net/base/net_errors.h"
#include "net/url_request/network_error_logging_delegate.h"

namespace net {
class ReportingService;
}  // namespace net

namespace url {
class Origin;
}  // namespace url

namespace features {
extern const base::Feature NETWORK_ERROR_LOGGING_EXPORT kNetworkErrorLogging;
}  // namespace features

namespace network_error_logging {

class NETWORK_ERROR_LOGGING_EXPORT NetworkErrorLoggingService
    : public net::NetworkErrorLoggingDelegate {
 public:
  static const char kReportType[];

  // Creates the NetworkErrorLoggingService.
  //
  // Will return nullptr if Network Error Logging is disabled via
  // base::FeatureList.
  static std::unique_ptr<NetworkErrorLoggingService> Create();

  // net::NetworkErrorLoggingDelegate implementation:

  ~NetworkErrorLoggingService() override;

  void SetReportingService(net::ReportingService* reporting_service) override;

  void OnHeader(const url::Origin& origin, const std::string& value) override;

  void OnNetworkError(const url::Origin& origin,
                      net::Error error,
                      ErrorDetailsCallback details_callback) override;

  void SetTickClockForTesting(std::unique_ptr<base::TickClock> tick_clock);

 private:
  // NEL Policy set by an origin.
  struct OriginPolicy {
    // Reporting API endpoint group to which reports should be sent.
    //
    // Note: This is a change from the current draft spec, which specifies a
    // list of URIs which should receive reports directly.
    std::string report_to;

    base::TimeTicks expires;

    bool include_subdomains;
  };

  // Would be unordered_map, but url::Origin has no hash.
  using PolicyMap = std::map<url::Origin, OriginPolicy>;
  using WildcardPolicyMap =
      std::map<std::string, std::set<const OriginPolicy*>>;

  NetworkErrorLoggingService();

  // Would be const, but base::TickClock::NowTicks isn't.
  bool ParseHeader(const std::string& json_value, OriginPolicy* policy_out);

  const OriginPolicy* FindPolicyForOrigin(const url::Origin& origin) const;
  const OriginPolicy* FindWildcardPolicyForDomain(
      const std::string& domain) const;
  void MaybeAddWildcardPolicy(const url::Origin& origin,
                              const OriginPolicy* policy);
  void MaybeRemoveWildcardPolicy(const url::Origin& origin,
                                 const OriginPolicy* policy);
  bool IsPolicyExpired(const OriginPolicy& policy) const;

  std::unique_ptr<base::TickClock> tick_clock_;

  // Unowned.
  net::ReportingService* reporting_service_;

  PolicyMap policies_;
  WildcardPolicyMap wildcard_policies_;

  DISALLOW_COPY_AND_ASSIGN(NetworkErrorLoggingService);
};

}  // namespace network_error_logging

#endif  // COMPONENTS_NETWORK_ERROR_LOGGING_NETWORK_ERROR_LOGGING_SERVICE_H_

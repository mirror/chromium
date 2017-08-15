// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/network_error_logging/network_error_logging_service.h"

#include <memory>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/time/default_tick_clock.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "net/base/ip_address.h"
#include "net/base/net_errors.h"
#include "net/reporting/reporting_service.h"
#include "net/socket/next_proto.h"
#include "net/url_request/network_error_logging_delegate.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace network_error_logging {

namespace {

const base::Feature kNetworkErrorLoggingFeature{
    "NetworkErrorLogging", base::FEATURE_DISABLED_BY_DEFAULT};

const char kReportToKey[] = "report-to";
const char kMaxAgeKey[] = "max-age";
const char kIncludeSubdomainsKey[] = "includeSubdomains";

// Returns the superdomain of a given domain, or the empty string if the given
// domain is just a single label. Note that this does not take into account
// anything like the Public Suffix List, so the superdomain may end up being a
// bare TLD.
//
// Examples:
//
// GetSuperdomain("assets.example.com") -> "example.com"
// GetSuperdomain("example.net") -> "net"
// GetSuperdomain("littlebox") -> ""
std::string GetSuperdomain(const std::string& domain) {
  size_t dot_pos = domain.find('.');
  if (dot_pos == std::string::npos)
    return "";

  return domain.substr(dot_pos + 1);
}

bool GetTypeFromNetError(net::Error error, std::string* type_out) {
  return false;
}

std::unique_ptr<const base::Value> CreateReportBody(
    const std::string& type,
    const net::NetworkErrorLoggingDelegate::ErrorDetails& details,
    base::TimeTicks now_ticks) {
  auto body = base::MakeUnique<base::DictionaryValue>();

  body->SetString("uri", details.uri.spec());
  body->SetString("referrer", details.referrer.spec());
  body->SetString("server-ip", details.server_ip.ToString());
  std::string protocol = net::NextProtoToString(details.protocol);
  if (protocol == "unknown")
    protocol = "";
  body->SetString("protocol", protocol);
  body->SetInteger("status-code", details.status_code);
  body->SetInteger("elapsed-time", details.elapsed_time.InMilliseconds());
  body->SetInteger("age", (now_ticks - details.time).InMilliseconds());
  body->SetString("type", type);

  return std::move(body);
}

}  // namespace

// static
std::unique_ptr<NetworkErrorLoggingService>
NetworkErrorLoggingService::Create() {
  if (!base::FeatureList::IsEnabled(kNetworkErrorLoggingFeature))
    return std::unique_ptr<NetworkErrorLoggingService>();

  // Would be MakeUnique, but the constructor is private so MakeUnique can't see
  // it.
  return base::WrapUnique(new NetworkErrorLoggingService());
}

NetworkErrorLoggingService::~NetworkErrorLoggingService() {}

void NetworkErrorLoggingService::SetReportingService(
    net::ReportingService* reporting_service) {
  reporting_service_ = reporting_service;
}

void NetworkErrorLoggingService::OnHeader(const url::Origin& origin,
                                          const std::string& value) {
  DCHECK(origin.GetURL().SchemeIsCryptographic());

  OriginPolicy policy;
  if (!ParseHeader(value, &policy))
    return;

  PolicyMap::iterator it = policies_.find(origin);
  if (it != policies_.end()) {
    MaybeRemoveWildcardPolicy(origin, &it->second);
    policies_.erase(it);
  }

  if (policy.expires.is_null())
    return;

  auto inserted = policies_.insert(std::make_pair(origin, policy));
  DCHECK(inserted.second);
  MaybeAddWildcardPolicy(origin, &inserted.first->second);
}

void NetworkErrorLoggingService::OnNetworkError(
    const url::Origin& origin,
    net::Error error,
    ErrorDetailsCallback details_callback) {
  if (!reporting_service_)
    return;

  const OriginPolicy* policy = FindPolicyForOrigin(origin);
  if (!policy)
    return;

  std::string type;
  if (!GetTypeFromNetError(error, &type))
    return;

  ErrorDetails details;
  std::move(details_callback).Run(&details);

  std::unique_ptr<const base::Value> body(
      CreateReportBody(type, details, tick_clock_->NowTicks()));

  // TODO more
}

void NetworkErrorLoggingService::SetTickClockForTesting(
    std::unique_ptr<base::TickClock> tick_clock) {
  DCHECK(tick_clock);
  tick_clock_ = std::move(tick_clock);
}

NetworkErrorLoggingService::NetworkErrorLoggingService()
    : tick_clock_(base::MakeUnique<base::DefaultTickClock>()),
      reporting_service_(nullptr) {}

bool NetworkErrorLoggingService::ParseHeader(const std::string& json_value,
                                             OriginPolicy* policy_out) {
  DCHECK(policy_out);

  std::unique_ptr<base::Value> value = base::JSONReader::Read(json_value);
  if (!value)
    return false;

  const base::DictionaryValue* dict = nullptr;
  if (!value->GetAsDictionary(&dict))
    return false;

  std::string report_to;
  if (!dict->GetString(kReportToKey, &report_to))
    return false;

  int max_age_sec;
  if (!dict->GetInteger(kMaxAgeKey, &max_age_sec) || max_age_sec < 0)
    return false;

  bool include_subdomains = false;
  // includeSubdomains is optional and defaults to false, so it's okay if
  // GetBoolean fails.
  dict->GetBoolean(kIncludeSubdomainsKey, &include_subdomains);

  policy_out->report_to = report_to;
  if (max_age_sec > 0)
    policy_out->expires =
        tick_clock_->NowTicks() + base::TimeDelta::FromSeconds(max_age_sec);
  else
    policy_out->expires = base::TimeTicks();
  policy_out->include_subdomains = include_subdomains;

  return true;
}

const NetworkErrorLoggingService::OriginPolicy*
NetworkErrorLoggingService::FindPolicyForOrigin(
    const url::Origin& origin) const {
  PolicyMap::const_iterator it = policies_.find(origin);
  if (it != policies_.end() && tick_clock_->NowTicks() < it->second.expires)
    return &it->second;

  std::string domain = origin.host();
  const OriginPolicy* wildcard_policy = nullptr;
  while (!wildcard_policy && !domain.empty()) {
    wildcard_policy = FindWildcardPolicyForDomain(domain);
    domain = GetSuperdomain(domain);
  }

  return wildcard_policy;
}

const NetworkErrorLoggingService::OriginPolicy*
NetworkErrorLoggingService::FindWildcardPolicyForDomain(
    const std::string& domain) const {
  DCHECK(!domain.empty());

  WildcardPolicyMap::const_iterator it = wildcard_policies_.find(domain);
  if (it == wildcard_policies_.end())
    return nullptr;

  DCHECK(!it->second.empty());

  // TODO(juliatuttle): Come up with a deterministic way to resolve these.
  if (it->second.size() > 1) {
    LOG(WARNING) << "Domain " << domain
                 << " matches multiple origins with includeSubdomains; "
                 << "choosing one arbitrarily.";
  }

  for (auto jt = it->second.begin(); jt != it->second.end(); ++jt) {
    if (tick_clock_->NowTicks() < (*jt)->expires)
      return *jt;
  }

  return nullptr;
}

void NetworkErrorLoggingService::MaybeAddWildcardPolicy(
    const url::Origin& origin,
    const OriginPolicy* policy) {
  DCHECK(policy);
  DCHECK_EQ(policy, &policies_[origin]);

  if (policy->include_subdomains)
    return;

  auto inserted = wildcard_policies_[origin.host()].insert(policy);
  DCHECK(inserted.second);
}

void NetworkErrorLoggingService::MaybeRemoveWildcardPolicy(
    const url::Origin& origin,
    const OriginPolicy* policy) {
  DCHECK(policy);
  DCHECK_EQ(policy, &policies_[origin]);

  if (!policy->include_subdomains)
    return;

  WildcardPolicyMap::iterator wildcard_it =
      wildcard_policies_.find(origin.host());
  DCHECK(wildcard_it != wildcard_policies_.end());

  size_t erased = wildcard_it->second.erase(policy);
  DCHECK_EQ(1u, erased);
  if (wildcard_it->second.empty())
    wildcard_policies_.erase(wildcard_it);
}

}  // namespace network_error_logging

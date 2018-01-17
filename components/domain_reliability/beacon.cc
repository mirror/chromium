// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/domain_reliability/beacon.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "components/domain_reliability/util.h"
#include "net/base/net_errors.h"

namespace domain_reliability {

using base::Value;
using base::DictionaryValue;

DomainReliabilityBeacon::DomainReliabilityBeacon() {}
DomainReliabilityBeacon::DomainReliabilityBeacon(
    const DomainReliabilityBeacon& other) = default;
DomainReliabilityBeacon::~DomainReliabilityBeacon() {}

std::unique_ptr<Value> DomainReliabilityBeacon::ToValue(
    base::TimeTicks upload_time,
    base::TimeTicks last_network_change_time,
    const GURL& collector_url,
    const std::vector<std::unique_ptr<std::string>>& path_prefixes) const {
  auto beacon_value = base::MakeUnique<DictionaryValue>();
  DCHECK(url.is_valid());
  GURL sanitized_url = SanitizeURLForReport(url, collector_url, path_prefixes);
  beacon_value->SetKey("url", base::Value(sanitized_url.spec()));
  beacon_value->SetKey("status", base::Value(status));
  if (!quic_error.empty())
    beacon_value->SetKey("quic_error", base::Value(quic_error));
  if (chrome_error != net::OK) {
    auto failure_value = base::MakeUnique<DictionaryValue>();
    failure_value->SetKey("custom_error",
                          base::Value(net::ErrorToString(chrome_error)));
    beacon_value->Set("failure_data", std::move(failure_value));
  }
  beacon_value->SetKey("server_ip", base::Value(server_ip));
  beacon_value->SetKey("was_proxied", base::Value(was_proxied));
  beacon_value->SetKey("protocol", base::Value(protocol));
  if (details.quic_broken)
    beacon_value->SetKey("quic_broken", base::Value(details.quic_broken));
  if (details.quic_port_migration_detected)
    beacon_value->SetKey("quic_port_migration_detected",
                         base::Value(details.quic_port_migration_detected));
  if (http_response_code >= 0)
    beacon_value->SetKey("http_response_code", base::Value(http_response_code));
  beacon_value->SetKey("request_elapsed_ms",
                       base::Value(static_cast<int>(elapsed.InMilliseconds())));
  base::TimeDelta request_age = upload_time - start_time;
  beacon_value->SetKey(
      "request_age_ms",
      base::Value(static_cast<int>(request_age.InMilliseconds())));
  bool network_changed = last_network_change_time > start_time;
  beacon_value->SetKey("network_changed", base::Value(network_changed));
  beacon_value->SetKey("sample_rate", base::Value(sample_rate));
  return std::move(beacon_value);
}

}  // namespace domain_reliability

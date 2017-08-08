// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/component_metrics_provider.h"

#include <map>
#include <string>
#include "base/strings/string_number_conversions.h"
#include "components/component_updater/component_updater_service.h"
#include "components/metrics/proto/system_profile.pb.h"

namespace metrics {

namespace {

const std::map<std::string, SystemProfileProto_ComponentId> component_map = {
    {"hfnkpimlhhgieaddgfemjhofmfblmnib",
     SystemProfileProto_ComponentId_CRL_SET},
    {"bjbdkfoakgmkndalgpadobhgbhhoanho",
     SystemProfileProto_ComponentId_EPSON_INKJET_PRINTER_ESCPR},
    {"khaoiebndkojlmppeemjhbpbandiljpe",
     SystemProfileProto_ComponentId_FILE_TYPE_POLICIES},
    {"mimojjlkmoijpicakmndhoigimigcmbb",
     SystemProfileProto_ComponentId_PEPPER_FLASH},
    {"ckjlcfmdbdglblbjglepgnoekdnkoklc",
     SystemProfileProto_ComponentId_PEPPER_FLASH_CHROMEOS},
    {"kfoklmclfodeliojeaekpoflbkkhojea",
     SystemProfileProto_ComponentId_ORIGIN_TRIALS},
    {"llkgjffcdpffmhiakmfcdcblohccpfmo",
     SystemProfileProto_ComponentId_ORIGIN_TRIALS},  // Alternate ID
    {"hnimpnehoodheedghdeeijklkeaacbdc", SystemProfileProto_ComponentId_PNACL},
    {"npdjjkjlcidkjlamlmmdelcjbcpdjocm",
     SystemProfileProto_ComponentId_RECOVERY},
    {"ojjgnpkioondelmggbekfhllhdaimnho",
     SystemProfileProto_ComponentId_STH_SET},
    {"gcmjkmgdlgnkkcocmoeiminaijmmjnii",
     SystemProfileProto_ComponentId_SUBRESOURCE_FILTER},
    {"gkmgaooipdjhmangpemjhigmamcehddo",
     SystemProfileProto_ComponentId_SW_REPORTER},
    {"giekcmmlnklenlaomppkphknjmnnpneh",
     SystemProfileProto_ComponentId_SSL_ERROR_ASSISTANT},
    {"oimompecagnajdejgnnjijobebaeigek",
     SystemProfileProto_ComponentId_WIDEVINE_CDM}};

SystemProfileProto_ComponentId CrxIdToComponentId(const std::string& comp) {
  const auto result = component_map.find(comp);
  if (result == component_map.end())
    return SystemProfileProto_ComponentId_UNKNOWN;
  return result->second;
}

// Extract the first 32 bits of a fingerprint string, excluding the fingerprint
// format specifier.
uint32_t Trim(const std::string& fp) {
  const auto len_prefix = fp.find(".");
  if (len_prefix == std::string::npos)
    return 0;
  uint32_t result = 0;
  if (base::HexStringToUInt(fp.substr(len_prefix + 1, 16), &result))
    return result;
  return 0;
}

}  // namespace

ComponentMetricsProvider::ComponentMetricsProvider(
    component_updater::ComponentUpdateService* cus)
    : cus_(cus) {}

ComponentMetricsProvider::~ComponentMetricsProvider() = default;

void ComponentMetricsProvider::ProvideSystemProfileMetrics(
    SystemProfileProto* system_profile) {
  for (const auto& component : cus_->GetComponents()) {
    const auto id = CrxIdToComponentId(component.id);
    // Ignore any unknown components - in practice these are the
    // SupervisedUserWhitelists, which we do not want to transmit to UMA or
    // Crash.
    if (id == SystemProfileProto_ComponentId_UNKNOWN)
      continue;
    auto* proto = system_profile->add_chrome_component();
    proto->set_component_id(id);
    proto->set_version(component.version.GetString());
    proto->set_omaha_fingerprint(Trim(component.fingerprint));
  }
}

}  // namespace metrics

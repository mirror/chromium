// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/component_metrics_provider.h"

#include <string>
#include "base/strings/string_number_conversions.h"
#include "components/component_updater/component_updater_service.h"
#include "components/metrics/proto/system_profile.pb.h"

namespace metrics {

namespace {

SystemProfileProto_ComponentId CrxIdToComponentId(const std::string& comp) {
  if (comp == "hfnkpimlhhgieaddgfemjhofmfblmnib") {
    return SystemProfileProto_ComponentId_CRL_SET;
  } else if (comp == "bjbdkfoakgmkndalgpadobhgbhhoanho") {
    return SystemProfileProto_ComponentId_EPSON_INKJET_PRINTER_ESCPR;
  } else if (comp == "khaoiebndkojlmppeemjhbpbandiljpe") {
    return SystemProfileProto_ComponentId_FILE_TYPE_POLICIES;
  } else if (comp == "mimojjlkmoijpicakmndhoigimigcmbb") {
    return SystemProfileProto_ComponentId_PEPPER_FLASH;
  } else if (comp == "ckjlcfmdbdglblbjglepgnoekdnkoklc") {
    return SystemProfileProto_ComponentId_PEPPER_FLASH_CHROMEOS;
  } else if (comp == "kfoklmclfodeliojeaekpoflbkkhojea" ||
             comp == "llkgjffcdpffmhiakmfcdcblohccpfmo") {
    return SystemProfileProto_ComponentId_ORIGIN_TRIALS;
  } else if (comp == "hnimpnehoodheedghdeeijklkeaacbdc") {
    return SystemProfileProto_ComponentId_PNACL;
  } else if (comp == "npdjjkjlcidkjlamlmmdelcjbcpdjocm") {
    return SystemProfileProto_ComponentId_RECOVERY;
  } else if (comp == "ojjgnpkioondelmggbekfhllhdaimnho") {
    return SystemProfileProto_ComponentId_STH_SET;
  } else if (comp == "gcmjkmgdlgnkkcocmoeiminaijmmjnii") {
    return SystemProfileProto_ComponentId_SUBRESOURCE_FILTER;
  } else if (comp == "gkmgaooipdjhmangpemjhigmamcehddo") {
    return SystemProfileProto_ComponentId_SW_REPORTER;
  } else if (comp == "giekcmmlnklenlaomppkphknjmnnpneh") {
    return SystemProfileProto_ComponentId_SSL_ERROR_ASSISTANT;
  } else if (comp == "oimompecagnajdejgnnjijobebaeigek") {
    return SystemProfileProto_ComponentId_WIDEVINE_CDM;
  }
  return SystemProfileProto_ComponentId_UNKNOWN;
}

uint32_t Trim(const std::string& fp) {
  auto l = fp.find(".");
  if (l == std::string::npos)
    return 0;
  uint32_t result = 0;
  if (base::HexStringToUInt(fp.substr(l + 1, 16), &result))
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
    auto id = CrxIdToComponentId(component.id);
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

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/network_service.mojom.h"
#include "net/proxy/proxy_bypass_rules.h"
#include "net/proxy/proxy_config.h"
#include "net/proxy/proxy_config_source.h"
#include "net/proxy/proxy_list.h"
#include "url/mojo/url.mojom.h"

namespace mojo {

template <>
class StructTraits<content::mojom::ProxyBypassRulesDataView,
                   net::ProxyBypassRules> {
 public:
  static std::string bypass_string(const net::ProxyBypassRules& r) {
    return r.ToString();
  }
  static bool Read(content::mojom::ProxyBypassRulesDataView data,
                   net::ProxyBypassRules* out_proxy_bypass_rules) {
    std::string bypass_string;
    if (!data.ReadBypassString(&bypass_string))
      return false;
    out_proxy_bypass_rules->ParseFromString(bypass_string);
    return true;
  }
};

template <>
class StructTraits<content::mojom::ProxyListDataView, net::ProxyList> {
 public:
  static std::string pac_string(const net::ProxyList& r) {
    return r.ToPacString();
  }
  static bool Read(content::mojom::ProxyListDataView data,
                   net::ProxyList* out_proxy_list) {
    std::string pac_string;
    if (!data.ReadPacString(&pac_string))
      return false;
    out_proxy_list->SetFromPacString(pac_string);
    return true;
  }
};

template <>
class EnumTraits<content::mojom::ProxyRulesType,
                 net::ProxyConfig::ProxyRules::Type> {
 public:
  static content::mojom::ProxyRulesType ToMojom(
      net::ProxyConfig::ProxyRules::Type net_proxy_rules_type);
  static bool FromMojom(content::mojom::ProxyRulesType mojo_proxy_rules_type,
                        net::ProxyConfig::ProxyRules::Type* out);
};

template <>
class StructTraits<content::mojom::ProxyRulesDataView,
                   net::ProxyConfig::ProxyRules> {
 public:
  static const net::ProxyBypassRules& bypass_rules(
      const net::ProxyConfig::ProxyRules& r) {
    return r.bypass_rules;
  }
  static bool reverse_bypass(const net::ProxyConfig::ProxyRules& r) {
    return r.reverse_bypass;
  }
  static net::ProxyConfig::ProxyRules::Type type(
      const net::ProxyConfig::ProxyRules& r) {
    return r.type;
  }
  static const net::ProxyList& single_proxies(
      const net::ProxyConfig::ProxyRules& r) {
    return r.single_proxies;
  }
  static const net::ProxyList& proxies_for_http(
      const net::ProxyConfig::ProxyRules& r) {
    return r.proxies_for_http;
  }
  static const net::ProxyList& proxies_for_https(
      const net::ProxyConfig::ProxyRules& r) {
    return r.proxies_for_https;
  }
  static const net::ProxyList& proxies_for_ftp(
      const net::ProxyConfig::ProxyRules& r) {
    return r.proxies_for_ftp;
  }
  static const net::ProxyList& fallback_proxies(
      const net::ProxyConfig::ProxyRules& r) {
    return r.fallback_proxies;
  }

  static bool Read(content::mojom::ProxyRulesDataView data,
                   net::ProxyConfig::ProxyRules* out_proxy_rules);
};

template <>
class EnumTraits<content::mojom::ProxyConfigSource, net::ProxyConfigSource> {
 public:
  static content::mojom::ProxyConfigSource ToMojom(
      net::ProxyConfigSource net_proxy_config_source);
  static bool FromMojom(
      content::mojom::ProxyConfigSource mojo_proxy_config_source,
      net::ProxyConfigSource* out);
};

template <>
class StructTraits<content::mojom::ProxyConfigDataView, net::ProxyConfig> {
 public:
  static bool auto_detect(const net::ProxyConfig& r) { return r.auto_detect(); }
  static const GURL& pac_url(const net::ProxyConfig& r) { return r.pac_url(); }
  static bool pac_mandatory(const net::ProxyConfig& r) {
    return r.pac_mandatory();
  }
  static const net::ProxyConfig::ProxyRules& proxy_rules(
      const net::ProxyConfig& r) {
    return r.proxy_rules();
  }
  static net::ProxyConfigSource source(const net::ProxyConfig& r) {
    return r.source();
  }
  static int32_t id(const net::ProxyConfig& r) { return r.id(); }
  static bool Read(content::mojom::ProxyConfigDataView data,
                   net::ProxyConfig* out_proxy_config);
};

}  // namespace mojo

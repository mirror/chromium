// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_SSL_CONFIG_TRAITS_H_
#define CONTENT_COMMON_SSL_CONFIG_TRAITS_H_

#include <stdint.h>

#include <vector>

#include "content/common/content_export.h"
#include "content/public/common/ssl_config.mojom.h"
#include "net/ssl/ssl_config.h"
#include "storage/common/blob_storage/blob_handle.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "ui/accessibility/ax_modes.h"

namespace mojo {

template <>
struct CONTENT_EXPORT
    EnumTraits<content::mojom::TLS13Variant, net::TLS13Variant> {
  static content::mojom::TLS13Variant ToMojom(net::TLS13Variant net_variant);
  static bool FromMojom(content::mojom::TLS13Variant mojo_variant,
                        net::TLS13Variant* net_variant);
};

template <>
struct CONTENT_EXPORT
    StructTraits<content::mojom::SSLConfigDataView, net::SSLConfig> {
 public:
  static bool rev_checking_enabled(const net::SSLConfig& ssl_config) {
    return ssl_config.rev_checking_enabled;
  }

  static bool rev_checking_required_local_anchors(
      const net::SSLConfig& ssl_config) {
    return ssl_config.rev_checking_required_local_anchors;
  }

  static bool sha1_local_anchors_enabled(const net::SSLConfig& ssl_config) {
    return ssl_config.sha1_local_anchors_enabled;
  }

  static bool common_name_fallback_local_anchors_enabled(
      const net::SSLConfig& ssl_config) {
    return ssl_config.common_name_fallback_local_anchors_enabled;
  }

  static uint16_t version_min(const net::SSLConfig& ssl_config) {
    return ssl_config.version_min;
  }

  static uint16_t version_max(const net::SSLConfig& ssl_config) {
    return ssl_config.version_max;
  }

  static net::TLS13Variant tls13_variant(const net::SSLConfig& ssl_config) {
    return ssl_config.tls13_variant;
  }

  static const std::vector<uint16_t>& disabled_cipher_suites(
      const net::SSLConfig& ssl_config) {
    return ssl_config.disabled_cipher_suites;
  }

  static bool channel_id_enabled(const net::SSLConfig& ssl_config) {
    return ssl_config.channel_id_enabled;
  }

  static bool false_start_enabled(const net::SSLConfig& ssl_config) {
    return ssl_config.false_start_enabled;
  }

  static bool require_ecdhe(const net::SSLConfig& ssl_config) {
    return ssl_config.require_ecdhe;
  }

  static bool Read(content::mojom::SSLConfigDataView& mojo_config,
                   net::SSLConfig* net_config);
};

}  // namespace mojo

#endif  // CONTENT_COMMON_SSL_CONFIG_TRAITS_H_

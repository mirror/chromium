// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/ssl_config_traits.h"

#include "base/logging.h"

namespace mojo {

content::mojom::TLS13Variant
EnumTraits<content::mojom::TLS13Variant, net::TLS13Variant>::ToMojom(
    net::TLS13Variant net_variant) {
  switch (net_variant) {
    case net::kTLS13VariantDraft:
      return content::mojom::TLS13Variant::kTLS13VariantDraft;
    case net::kTLS13VariantExperiment:
      return content::mojom::TLS13Variant::kTLS13VariantExperiment;
    case net::kTLS13VariantRecordTypeExperiment:
      return content::mojom::TLS13Variant::kTLS13VariantRecordTypeExperiment;
    case net::kTLS13VariantNoSessionIDExperiment:
      return content::mojom::TLS13Variant::kTLS13VariantNoSessionIDExperiment;
  }

  NOTREACHED();
  return content::mojom::TLS13Variant::kTLS13VariantDraft;
}

bool EnumTraits<content::mojom::TLS13Variant, net::TLS13Variant>::FromMojom(
    content::mojom::TLS13Variant mojo_variant,
    net::TLS13Variant* net_variant) {
  switch (mojo_variant) {
    case content::mojom::TLS13Variant::kTLS13VariantDraft:
      *net_variant = net::kTLS13VariantDraft;
      return true;
    case content::mojom::TLS13Variant::kTLS13VariantExperiment:
      *net_variant = net::kTLS13VariantExperiment;
      return true;
    case content::mojom::TLS13Variant::kTLS13VariantRecordTypeExperiment:
      *net_variant = net::kTLS13VariantRecordTypeExperiment;
      return true;
    case content::mojom::TLS13Variant::kTLS13VariantNoSessionIDExperiment:
      *net_variant = net::kTLS13VariantNoSessionIDExperiment;
      return true;
  }

  return false;
}

bool StructTraits<content::mojom::SSLConfigDataView, net::SSLConfig>::Read(
    content::mojom::SSLConfigDataView& mojo_config,
    net::SSLConfig* net_config) {
  *net_config = net::SSLConfig();
  if (!mojo_config.ReadTls13Variant(&net_config->tls13_variant) ||
      !mojo_config.ReadDisabledCipherSuites(
          &net_config->disabled_cipher_suites)) {
    return false;
  }
  net_config->rev_checking_enabled = mojo_config.rev_checking_enabled();
  net_config->rev_checking_required_local_anchors =
      mojo_config.rev_checking_required_local_anchors();
  net_config->sha1_local_anchors_enabled =
      mojo_config.sha1_local_anchors_enabled();
  net_config->common_name_fallback_local_anchors_enabled =
      mojo_config.common_name_fallback_local_anchors_enabled();
  net_config->version_min = mojo_config.version_min();
  net_config->version_max = mojo_config.version_max();
  net_config->channel_id_enabled = mojo_config.channel_id_enabled();
  net_config->false_start_enabled = mojo_config.false_start_enabled();
  net_config->require_ecdhe = mojo_config.require_ecdhe();
  return true;
}

}  // namespace mojo

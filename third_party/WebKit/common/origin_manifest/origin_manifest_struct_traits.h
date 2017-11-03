// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_COMMON_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STRUCT_TRAITS_H_
#define THIRD_PARTY_WEBKIT_COMMON_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STRUCT_TRAITS_H_

#include "third_party/WebKit/common/origin_manifest/origin_manifest.h"
#include "third_party/WebKit/common/origin_manifest/origin_manifest.mojom.h"

namespace mojo {

template <>
struct EnumTraits<::blink::mojom::OriginManifestContentSecurityPolicyType,
                  ::blink::OriginManifest::ContentSecurityPolicyType> {
  static ::blink::mojom::OriginManifestContentSecurityPolicyType ToMojom(
      ::blink::OriginManifest::ContentSecurityPolicyType input) {
    switch (input) {
      case ::blink::OriginManifest::ContentSecurityPolicyType::kReport:
        return ::blink::mojom::OriginManifestContentSecurityPolicyType::kReport;
      case ::blink::OriginManifest::ContentSecurityPolicyType::kEnforce:
        return ::blink::mojom::OriginManifestContentSecurityPolicyType::
            kEnforce;
    }
    NOTREACHED();
    return ::blink::mojom::OriginManifestContentSecurityPolicyType::kEnforce;
  }

  static bool FromMojom(
      ::blink::mojom::OriginManifestContentSecurityPolicyType input,
      ::blink::OriginManifest::ContentSecurityPolicyType* output) {
    switch (input) {
      case ::blink::mojom::OriginManifestContentSecurityPolicyType::kReport:
        *output = ::blink::OriginManifest::ContentSecurityPolicyType::kReport;
        return true;
      case ::blink::mojom::OriginManifestContentSecurityPolicyType::kEnforce:
        *output = ::blink::OriginManifest::ContentSecurityPolicyType::kEnforce;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
class StructTraits<blink::mojom::OriginManifestContentSecurityPolicyDataView,
                   blink::OriginManifest::ContentSecurityPolicy> {
 public:
  static std::string policy(
      const blink::OriginManifest::ContentSecurityPolicy& csp) {
    return csp.policy;
  }
  static blink::OriginManifest::ContentSecurityPolicyType disposition(
      const blink::OriginManifest::ContentSecurityPolicy& csp);

  static bool Read(
      blink::mojom::OriginManifestContentSecurityPolicyDataView data,
      blink::OriginManifest::ContentSecurityPolicy* output);
};

template <>
class StructTraits<blink::mojom::OriginManifestDataView,
                   blink::OriginManifest> {
 public:
  static const std::vector<blink::OriginManifest::ContentSecurityPolicy>&
  csp_baseline(const blink::OriginManifest& om);
  static const std::vector<blink::OriginManifest::ContentSecurityPolicy>&
  csp_fallback(const blink::OriginManifest& om);

  static bool Read(blink::mojom::OriginManifestDataView data,
                   blink::OriginManifest* output);
};

}  // namespace mojo

#endif  // THIRD_PARTY_WEBKIT_COMMON_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STRUCT_TRAITS_H_

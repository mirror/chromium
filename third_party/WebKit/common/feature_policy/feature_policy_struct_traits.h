// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_COMMON_FEATURE_POLICY_FEATURE_POLICY_STRUCT_TRAITS_H_
#define THIRD_PARTY_WEBKIT_COMMON_FEATURE_POLICY_FEATURE_POLICY_STRUCT_TRAITS_H_

#include "mojo/public/cpp/bindings/enum_traits.h"

#include <vector>

#include "third_party/WebKit/common/common_export.h"
#include "third_party/WebKit/common/feature_policy/feature_policy.h"
#include "third_party/WebKit/common/feature_policy/feature_policy.mojom.h"
#include "third_party/WebKit/common/feature_policy/feature_policy_feature.h"

namespace mojo {

template <>
struct BLINK_COMMON_EXPORT EnumTraits<blink::mojom::FeaturePolicyFeature,
                                      blink::FeaturePolicyFeature> {
  static blink::mojom::FeaturePolicyFeature ToMojom(
      blink::FeaturePolicyFeature feature);
  static bool FromMojom(blink::mojom::FeaturePolicyFeature in,
                        blink::FeaturePolicyFeature* out);
};

template <>
class BLINK_COMMON_EXPORT
    StructTraits<blink::mojom::ParsedFeaturePolicyDeclarationDataView,
                 blink::ParsedFeaturePolicyDeclaration> {
 public:
  static blink::FeaturePolicyFeature feature(
      const blink::ParsedFeaturePolicyDeclaration& policy) {
    return policy.feature;
  }
  static bool matches_all_origins(
      const blink::ParsedFeaturePolicyDeclaration& policy) {
    return policy.matches_all_origins;
  }
  static const std::vector<url::Origin>& origins(
      const blink::ParsedFeaturePolicyDeclaration& policy) {
    return policy.origins;
  }

  static bool Read(blink::mojom::ParsedFeaturePolicyDeclarationDataView in,
                   blink::ParsedFeaturePolicyDeclaration* out);
};

}  // namespace mojo

#endif  // THIRD_PARTY_WEBKIT_COMMON_FEATURE_POLICY_FEATURE_POLICY_STRUCT_TRAITS_H_

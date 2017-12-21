// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_REFERRER_POLICY_H_
#define SERVICES_NETWORK_PUBLIC_CPP_REFERRER_POLICY_H_

namespace blink {

// These values are serialized and persisted, so do not remove values and add
// new ones at the end.
// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.blink_public.web
// GENERATED_JAVA_CLASS_NAME_OVERRIDE: WebReferrerPolicy
enum WebReferrerPolicy {
  kWebReferrerPolicyAlways,
  kWebReferrerPolicyDefault,
  kWebReferrerPolicyNoReferrerWhenDowngrade,
  kWebReferrerPolicyNever,
  kWebReferrerPolicyOrigin,
  kWebReferrerPolicyOriginWhenCrossOrigin,
  // This policy corresponds to strict-origin-when-cross-origin.
  // TODO(estark): rename to match the spec.
  kWebReferrerPolicyNoReferrerWhenDowngradeOriginWhenCrossOrigin,
  kWebReferrerPolicySameOrigin,
  kWebReferrerPolicyStrictOrigin,
  kWebReferrerPolicyLast = kWebReferrerPolicyStrictOrigin
};

}  // namespace blink

#endif  // SERVICES_NETWORK_PUBLIC_CPP_REFERRER_POLICY_H_

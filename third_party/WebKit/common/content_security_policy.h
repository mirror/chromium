// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_COMMON_CONTENT_SECURITY_POLICY_H_
#define THIRD_PARTY_WEBKIT_COMMON_CONTENT_SECURITY_POLICY_H_

namespace blink {

enum ContentSecurityPolicyType {
  kContentSecurityPolicyTypeReport,
  kContentSecurityPolicyTypeEnforce,
  kContentSecurityPolicyTypeLast = kContentSecurityPolicyTypeEnforce
};

enum ContentSecurityPolicySource {
  kContentSecurityPolicySourceHTTP,
  kContentSecurityPolicySourceMeta,
  kContentSecurityPolicySourceLast = kContentSecurityPolicySourceMeta
};

enum ContentSecurityPolicyDisposition {
  kContentSecurityPolicyDispositionDoNotCheck,
  kContentSecurityPolicyDispositionCheck,
  kContentSecurityPolicyDispositionLast = kContentSecurityPolicyDispositionCheck
};

}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_COMMON_CONTENT_SECURITY_POLICY_H_

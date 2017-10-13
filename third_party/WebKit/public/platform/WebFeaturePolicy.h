// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebFeaturePolicy_h
#define WebFeaturePolicy_h

#include "WebCommon.h"
#include "WebFeaturePolicyFeature.h"
#include "WebSecurityOrigin.h"
#include "WebString.h"
#include "WebVector.h"
#include "url/origin.h"

namespace blink {

struct BLINK_PLATFORM_EXPORT WebParsedFeaturePolicyDeclaration {
  WebParsedFeaturePolicyDeclaration() : matches_all_origins(false) {}
  WebFeaturePolicyFeature feature;
  bool matches_all_origins;
  WebVector<WebSecurityOrigin> origins;
};

// Used in Blink code to represent parsed headers. Used for IPC between renderer
// and browser.
using WebParsedFeaturePolicy = WebVector<WebParsedFeaturePolicyDeclaration>;

// Composed full policy for a document. Stored in SecurityContext for each
// document. This is essentially an opaque handle to an object in the embedder.
class BLINK_PLATFORM_EXPORT WebFeaturePolicy {
 public:
  virtual ~WebFeaturePolicy() {}

  // Returns whether or not the given feature is enabled for the origin of the
  // document that owns the policy.
  virtual bool IsFeatureEnabled(blink::WebFeaturePolicyFeature) const = 0;
  // Returns whether or not the given feature is enabled by this policy for a
  // specific origin.
  virtual bool IsFeatureEnabledForOrigin(blink::WebFeaturePolicyFeature,
                                         const url::Origin&) const = 0;
  // Returns a list of explicitly-named origins of the given feature, it might
  // return an empty list even though the feature is enabled for all origins.
  // This method should be used together with IsFeatureEnabledForAll to get the
  // complete whitelist of the feature.
  virtual std::vector<url::Origin> GetOriginsForFeature(
      blink::WebFeaturePolicyFeature) const = 0;
  // Returns whether or not the given feature is enabled for all origins of the
  // document that owns the policy.
  virtual bool IsFeatureEnabledForAll(blink::WebFeaturePolicyFeature) const = 0;
};

}  // namespace blink

#endif  // WebFeaturePolicy_h

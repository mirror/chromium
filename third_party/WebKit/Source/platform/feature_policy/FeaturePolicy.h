// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FeaturePolicy_h
#define FeaturePolicy_h

#include "platform/PlatformExport.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "platform/wtf/HashMap.h"
#include "platform/wtf/RefPtr.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/text/StringHash.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/WebFeaturePolicy.h"

#include <memory>

namespace blink {

// Returns a map between feature name (string) and WebFeaturePolicyFeature
// (enum).
typedef HashMap<String, WebFeaturePolicyFeature> FeatureNameMap;
PLATFORM_EXPORT const FeatureNameMap& GetDefaultFeatureNameMap();

// Converts a JSON feature policy string into a vector of whitelists, one for
// each feature specified. Unrecognized features are filtered out. If |messages|
// is not null, then any errors in the input will cause an error message to be
// appended to it.
PLATFORM_EXPORT WebParsedFeaturePolicy
ParseFeaturePolicy(const String& policy,
                   RefPtr<SecurityOrigin>,
                   Vector<String>* messages);

// Converts a JSON feature policy string into a vector of whitelists (see
// comments above), with an explicit FeatureNameMap. This method is primarily
// used for testing.
PLATFORM_EXPORT WebParsedFeaturePolicy
ParseFeaturePolicy(const String& policy,
                   RefPtr<SecurityOrigin>,
                   Vector<String>* messages,
                   const FeatureNameMap& feature_names);

// Verifies whether feature policy is enabled and |feature| is supported in
// feature policy.
PLATFORM_EXPORT bool IsSupportedInFeaturePolicy(WebFeaturePolicyFeature);

// Methods for easily manipulating policies. These take a Vector rather than a
// WebVector, since they are intended to be used while the policy structure is
// being constructed.

// Returns true iff any declaration in the policy is for the given feature.
PLATFORM_EXPORT bool IsFeatureDeclared(
    const WebFeaturePolicyFeature,
    const Vector<WebParsedFeaturePolicyDeclaration>&);

// If no declaration in the policy exists already for the feature, adds a
// declaration which disallows the feature in all origins.
PLATFORM_EXPORT void DisallowFeatureIfNotPresent(
    const WebFeaturePolicyFeature,
    Vector<WebParsedFeaturePolicyDeclaration>*);

// If no declaration in the policy exists already for the feature, adds a
// declaration which allows the feature in all origins.
PLATFORM_EXPORT void AllowFeatureEverywhereIfNotPresent(
    const WebFeaturePolicyFeature,
    Vector<WebParsedFeaturePolicyDeclaration>*);

}  // namespace blink

#endif  // FeaturePolicy_h

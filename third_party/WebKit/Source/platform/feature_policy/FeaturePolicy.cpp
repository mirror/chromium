// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/feature_policy/FeaturePolicy.h"

#include "platform/RuntimeEnabledFeatures.h"
#include "platform/json/JSONValues.h"
#include "platform/network/HTTPParsers.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "platform/wtf/BitVector.h"
#include "platform/wtf/PtrUtil.h"

namespace blink {

WebParsedFeaturePolicy ParseFeaturePolicy(const String& policy,
                                          RefPtr<SecurityOrigin> origin,
                                          Vector<String>* messages) {
  return ParseContainerPolicy(policy, origin, RefPtr<SecurityOrigin>(),
                              messages, GetDefaultFeatureNameMap());
}

Vector<WebParsedFeaturePolicyDeclaration> ParseContainerPolicy(
    const String& policy,
    RefPtr<SecurityOrigin> self_origin,
    RefPtr<SecurityOrigin> src_origin,
    Vector<String>* messages) {
  return ParseContainerPolicy(policy, self_origin, src_origin, messages,
                              GetDefaultFeatureNameMap());
}

Vector<WebParsedFeaturePolicyDeclaration> ParseContainerPolicy(
    const String& policy,
    RefPtr<SecurityOrigin> self_origin,
    RefPtr<SecurityOrigin> src_origin,
    Vector<String>* messages,
    const FeatureNameMap& feature_names) {
  Vector<WebParsedFeaturePolicyDeclaration> whitelists;
  BitVector features_specified(
      static_cast<int>(WebFeaturePolicyFeature::LAST_FEATURE));

  // RFC2616, section 4.2 specifies that headers appearing multiple times can be
  // combined with a comma. Walk the header string, and parse each comma
  // separated chunk as a separate header.
  Vector<String> policy_items;
  // policy_items = [ policy *( "," [ policy ] ) ]
  policy.Split(',', policy_items);
  for (const String& item : policy_items) {
    Vector<String> entry_list;
    // entry_list = [ entry *( ";" [ entry ] ) ]
    item.Split(';', entry_list);
    for (const String& entry : entry_list) {
      // Split removes extra whitespaces by default
      //     "name value1 value2" or "name".
      Vector<String> tokens;
      entry.Split(' ', tokens);
      if (!feature_names.Contains(tokens[0])) {
        messages->push_back("Unrecognized feature: '" + tokens[0] + "'.");
        continue;
      }

      WebFeaturePolicyFeature feature = feature_names.at(tokens[0]);
      // If a policy has already been specified for the current feature, drop
      // the new policy.
      if (features_specified.QuickGet(static_cast<int>(feature)))
        continue;

      WebParsedFeaturePolicyDeclaration whitelist;
      whitelist.feature = feature;
      features_specified.QuickSet(static_cast<int>(feature));
      Vector<WebSecurityOrigin> origins;
      // If a policy entry has no (optional) values, only valid syntax for allow
      // attribute (e,g, allow="feature_name1; feature_name2 value"), enable the
      // feature for src origin.
      if (tokens.size() == 1) {
        DCHECK(src_origin);
        origins.push_back(src_origin);
      }

      for (size_t i = 1; i < tokens.size(); i++) {
        if (EqualIgnoringASCIICase(tokens[i], "'self'")) {
          origins.push_back(self_origin);
        } else if (EqualIgnoringASCIICase(tokens[i], "'src'")) {
          origins.push_back(src_origin);
        } else if (EqualIgnoringASCIICase(tokens[i], "'none'")) {
          continue;
        } else if (tokens[i] == "*") {
          whitelist.matches_all_origins = true;
          break;
        } else {
          WebSecurityOrigin target_origin =
              WebSecurityOrigin::CreateFromString(tokens[i]);
          if (!target_origin.IsNull() && !target_origin.IsUnique()) {
            origins.push_back(target_origin);
          } else {
            messages->push_back("Unrecognized origin: '" + tokens[i] + "'.");
          }
        }
      }
      whitelist.origins = origins;
      whitelists.push_back(whitelist);
    }
  }
  return whitelists;
}

bool IsSupportedInFeaturePolicy(WebFeaturePolicyFeature feature) {
  if (!RuntimeEnabledFeatures::FeaturePolicyEnabled())
    return false;

  switch (feature) {
    // TODO(loonybear): Re-enabled fullscreen in feature policy once tests have
    // been updated.
    // crbug.com/666761
    case WebFeaturePolicyFeature::kFullscreen:
      return false;
    case WebFeaturePolicyFeature::kPayment:
    case WebFeaturePolicyFeature::kUsb:
      return true;
    case WebFeaturePolicyFeature::kVibrate:
      return RuntimeEnabledFeatures::FeaturePolicyExperimentalFeaturesEnabled();
    default:
      return false;
  }
}

const FeatureNameMap& GetDefaultFeatureNameMap() {
  DEFINE_STATIC_LOCAL(FeatureNameMap, default_feature_name_map, ());
  if (default_feature_name_map.IsEmpty()) {
    default_feature_name_map.Set("fullscreen",
                                 WebFeaturePolicyFeature::kFullscreen);
    default_feature_name_map.Set("payment", WebFeaturePolicyFeature::kPayment);
    default_feature_name_map.Set("usb", WebFeaturePolicyFeature::kUsb);
    default_feature_name_map.Set("camera", WebFeaturePolicyFeature::kCamera);
    default_feature_name_map.Set("encrypted-media",
                                 WebFeaturePolicyFeature::kEme);
    default_feature_name_map.Set("microphone",
                                 WebFeaturePolicyFeature::kMicrophone);
    default_feature_name_map.Set("speaker", WebFeaturePolicyFeature::kSpeaker);
    default_feature_name_map.Set("geolocation",
                                 WebFeaturePolicyFeature::kGeolocation);
    default_feature_name_map.Set("midi", WebFeaturePolicyFeature::kMidiFeature);
    if (RuntimeEnabledFeatures::FeaturePolicyExperimentalFeaturesEnabled()) {
      default_feature_name_map.Set("vibrate",
                                   WebFeaturePolicyFeature::kVibrate);
      default_feature_name_map.Set("cookie",
                                   WebFeaturePolicyFeature::kDocumentCookie);
      default_feature_name_map.Set("domain",
                                   WebFeaturePolicyFeature::kDocumentDomain);
      default_feature_name_map.Set("docwrite",
                                   WebFeaturePolicyFeature::kDocumentWrite);
      default_feature_name_map.Set("sync-script",
                                   WebFeaturePolicyFeature::kSyncScript);
      default_feature_name_map.Set("sync-xhr",
                                   WebFeaturePolicyFeature::kSyncXHR);
    }
  }
  return default_feature_name_map;
}

}  // namespace blink

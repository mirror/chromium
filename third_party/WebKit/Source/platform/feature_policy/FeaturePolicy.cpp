// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/feature_policy/FeaturePolicy.h"

#include "platform/RuntimeEnabledFeatures.h"
#include "platform/json/JSONValues.h"
#include "platform/network/HTTPParsers.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "platform/wtf/PtrUtil.h"

namespace blink {

WebParsedFeaturePolicy ParseFeaturePolicy(const String& policy,
                                          RefPtr<SecurityOrigin> origin,
                                          Vector<String>* messages) {
  return ParseFeaturePolicy(policy, origin, messages,
                            GetDefaultFeatureNameMap());
}

WebParsedFeaturePolicy ParseFeaturePolicy(const String& policy,
                                          RefPtr<SecurityOrigin> origin,
                                          Vector<String>* messages,
                                          const FeatureNameMap& feature_names) {
  Vector<WebParsedFeaturePolicyDeclaration> whitelists;

  // Use a reasonable parse depth limit; the actual maximum depth is only going
  // to be 4 for a valid policy, but we'll give the featurePolicyParser a chance
  // to report more specific errors, unless the string is really invalid.
  std::unique_ptr<JSONArray> policy_items = ParseJSONHeader(policy, 50);
  if (!policy_items) {
    if (messages)
      messages->push_back("Unable to parse header.");
    return whitelists;
  }

  for (size_t i = 0; i < policy_items->size(); ++i) {
    JSONObject* item = JSONObject::Cast(policy_items->at(i));
    if (!item) {
      if (messages)
        messages->push_back("Policy is not an object.");
      continue;  // Array element is not an object; skip
    }

    for (size_t j = 0; j < item->size(); ++j) {
      JSONObject::Entry entry = item->at(j);
      if (!feature_names.Contains(entry.first))
        continue;  // Unrecognized feature; skip
      // TODO(lunalu): Add console warning for unrecognized feature.
      WebFeaturePolicyFeature feature = feature_names.at(entry.first);
      JSONArray* targets = JSONArray::Cast(entry.second);
      if (!targets) {
        if (messages)
          messages->push_back("Whitelist is not an array of strings.");
        continue;
      }

      WebParsedFeaturePolicyDeclaration whitelist;
      whitelist.feature = feature;
      Vector<WebSecurityOrigin> origins;
      String target_string;
      for (size_t j = 0; j < targets->size(); ++j) {
        if (targets->at(j)->AsString(&target_string)) {
          if (EqualIgnoringASCIICase(target_string, "self")) {
            if (!origin->IsUnique())
              origins.push_back(origin);
          } else if (target_string == "*") {
            whitelist.matches_all_origins = true;
          } else {
            WebSecurityOrigin target_origin =
                WebSecurityOrigin::CreateFromString(target_string);
            if (!target_origin.IsNull() && !target_origin.IsUnique())
              origins.push_back(target_origin);
            // TODO(lunalu): create console error if an origin does not parse
          }
        } else {
          if (messages)
            messages->push_back("Whitelist is not an array of strings.");
        }
      }
      whitelist.origins = origins;
      whitelists.push_back(whitelist);
    }
  }
  return whitelists;
}

bool IsValidFeaturePolicyAttr(const String& attr,
                              RefPtr<SecurityOrigin>& self_origin,
                              Vector<String>* console_warnings,
                              Vector<String>* console_errors) {
  WebParsedFeaturePolicy whitelists = ConstructFeaturePolicyFromHeaderValue(
      attr, self_origin, console_warnings, console_errors);
  if (!console_errors->IsEmpty())
    return false;
  return true;
}

WebParsedFeaturePolicy ConstructFeaturePolicyFromHeaderValue(
    const String& header,
    const RefPtr<SecurityOrigin>& origin,
    Vector<String>* warnings,
    Vector<String>* errors) {
  Vector<UChar> characters;
  header.AppendTo(characters);
  const UChar* begin = characters.data();
  const UChar* end = begin + characters.size();
  if (begin == end)
    return;
  const UChar* position = begin;
  Vector<WebParsedFeaturePolicyDeclaration> whitelists;
  while (position < end) {
    const UChar* whitelist_begin = position;
    SkipUntil<UChar>(position, end, ';');

    String name, value;
    if (ParseFPDeclaration(whitelist_begin, position, name, errors)) {
      WebFeaturePolicyFeature feature_name =
          GetDefaultFeatureNameMap().Contains(name)
              ? GetDefaultFeatureNameMap().at(name)
              : WebParsedFeaturePolicyFeature::kNotFound;
      if (feature_name == WebParsedFeaturePolicyFeature::kNotFound) {
        DCHECK(!warnings->IsNull());
        warnings->push_back("Unrecognized feature name: '" + name + "'.");
      } else {
        whitelists.push_back(
            CreateFPDeclaration(feature_name, origin, value, errors));
      }
    }

    SkipExactly<UChar>(position, end, ';');
  }
  return whitelists;
}

bool ParseFPDeclaration(const UChar* begin,
                        const UChar* end,
                        String& name,
                        String& value,
                        Vector<String>* errors) {
  DCHECK(name.IsEmpty());
  DCHECK(value.IsEmpty());

  const UChar* position = begin;
  SkipWhile<UChar, IsASCIISpace>(position, end);

  // Empty declaration (e.g. ";;;"). Exit early.
  if (position == end) {
    DCHECK(!errors->IsNull());
    errors->push_back("Feature policy declaration cannot be empty.");
    return false;
  }

  const UChar* name_begin = position;
  SkipWhile<UChar, IsCSPDirectiveNameCharacter>(position, end);

  if (name_begin == position) {
    SkipWhile<UChar, IsNotASCIISpace>(position, end);
    DCHECK(!errors->IsNull());
    errors->push_back("Invalid feature name '" +
                      String(name_begin, position - name_begin) + "'.");
    return false;
  }

  name = String(name_begin, position - name_begin);
  if (position == end)
    return true;

  if (!SkipExactly<UChar, IsASCIISpace>(position, end)) {
    SkipWhile<UChar, IsNotASCIISpace>(position, end);
    DCHECK(!errors->IsNull());
    errors->push_back("Invalid feature name '" +
                      String(name_begin, position - name_begin) + "'.");
    return false;
  }

  SkipWhile<UChar, IsASCIISpace>(position, end);
  const UChar* value_begin = position;
  SkipWhile<UChar, IsCSPDirectiveValueCharacter>(position, end);

  if (position != end) {
    DCHECK(!errors->IsNull());
    errors->push_back("The value for feature '" + name +
                      "' contains an invalid" + "character: '" +
                      String(value_begin, end - value_begin));
    return false;
  }

  // The declaration value may be empty (enabled on "src" origin).
  if (value_begin == position)
    return true;

  value = String(value_begin, position - value_begin);
  return true;
}

void CreateFPDeclaration(WebParsedFeaturePolicyFeature feature_name,
                         const RefPtr<SecurityOrigin>& self_origin,
                         const String& value,
                         Vector<String>* errors) {
  WebParsedFeaturePolicyDeclaration whitelist;
  whitelist.feature = feature_name;
  Vector<UChar> characters;
  value.AppendTo(characters);
  ParseOriginsFromValue(characters.data(),
                        characters.data() + characters.size(), &whitelist,
                        self_origin, errors);
}

void ParseOriginsFromValue(const UChar* begin,
                           const UChar* end,
                           WebParsedFeaturePolicyDeclaration* whitelist,
                           const RefPtr<WebSecurityOrigin>& self_origin,
                           Vector<String>* errors) {
  if (IsSourceListNone(begin, end))
    return;
  const UChar* position = begin;
  Vector<WebSecurityOrigin> origins;
  while (position < end) {
    SkipWhile<UChar, IsASCIISpace>(position, end);
    if (position == end)
      break;
    const UChar* origin_begin = position;
    SkipWhile < UChar, IsSourceCharacter(position, end);
    WebSecurityOrigin origin;
    bool matches_all_origins, is_self_origin = false;
    if (ParseOrigin(origin_begin, position, &matches_all_origins, origin)) {
      whitelist->origins.push_back(origin);
    } else if (matches_all_origins) {
      whitelist->matches_all_origins = true;
    } else if (is_self_origin) {
      whitelist->origins.push_back(self_origin);
    } else {
      DCHECK(!errors->IsNull());
      errors->push_back("Cannot parse origin :'" +
                        String(origin_begin, position) + "'.");
    }
    DCHECK(position == end || IsASCIISpace(*position));
  }
}

bool ParseOrigin(const UChar* begin,
                 const UChar* end,
                 bool* matches_all_origins,
                 WebSecurityOrigin& origin) {
  if (begin == end)
    return false;
  StringView token(begin, end - begin);
  if (EqualIgnoringASCIICase("'none'", token))
    return false;

  if (end - begin == 1 && *begin == '*') {
    *matches_all_origins = true;
    return false;
  }

  if (EqualIgnoringASCIICase("'self'", token)) {
    // origin <- self origin;
  }

  origin = WebSecurityOrigin::CreateFromString(String(begin, end));
  if (!origin.IsNull() && !origin.IsUnique())
    return true;

  return false;
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

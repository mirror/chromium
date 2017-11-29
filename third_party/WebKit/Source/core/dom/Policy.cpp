// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/Policy.h"

#include "core/dom/Document.h"
#include "core/inspector/ConsoleMessage.h"
#include "platform/feature_policy/FeaturePolicy.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "platform/wtf/text/StringUTF8Adaptor.h"

namespace blink {

// static
Policy* Policy::Create(Document* document) {
  return new Policy(document);
}

Policy* Policy::Create(Document* parent_document,
                       const ParsedFeaturePolicy& container_policy,
                       const scoped_refptr<SecurityOrigin> origin) {
  return new Policy(parent_document, container_policy, origin);
}

// explicit
Policy::Policy(Document* document) : document_(document) {
  ParsedFeaturePolicy container_policy;
  policy_ = FeaturePolicy::CreateFromParentPolicy(
      document_->GetFeaturePolicy(), container_policy,
      document_->GetSecurityOrigin()->ToUrlOrigin());
}

Policy::Policy(Document* parent_document,
               const ParsedFeaturePolicy& container_policy,
               const scoped_refptr<SecurityOrigin> origin)
    : document_(parent_document) {
  UpdateContainerPolicy(container_policy, origin);
}

bool Policy::allowsFeature(const String& feature) const {
  if (GetDefaultFeatureNameMap().Contains(feature)) {
    return policy_->IsFeatureEnabled(GetDefaultFeatureNameMap().at(feature));
  }

  AddWarningForUnrecognizedFeature(feature);
  return false;
}

bool Policy::allowsFeature(const String& feature, const String& url) const {
  const scoped_refptr<SecurityOrigin> origin =
      SecurityOrigin::CreateFromString(url);
  if (!origin || origin->IsUnique()) {
    document_->AddConsoleMessage(ConsoleMessage::Create(
        kOtherMessageSource, kWarningMessageLevel,
        "Invalid origin url for feature '" + feature + "': " + url + "."));
    return false;
  }

  if (!GetDefaultFeatureNameMap().Contains(feature)) {
    AddWarningForUnrecognizedFeature(feature);
    return false;
  }

  return policy_->IsFeatureEnabledForOrigin(
      GetDefaultFeatureNameMap().at(feature), origin->ToUrlOrigin());
}

Vector<String> Policy::allowedFeatures() const {
  Vector<String> allowed_features;
  for (const auto& entry : GetDefaultFeatureNameMap()) {
    if (policy_->IsFeatureEnabled(entry.value))
      allowed_features.push_back(entry.key);
  }
  return allowed_features;
}

Vector<String> Policy::getAllowlistForFeature(const String& feature) const {
  if (GetDefaultFeatureNameMap().Contains(feature)) {
    const FeaturePolicy::Whitelist whitelist =
        policy_->GetWhitelistForFeature(GetDefaultFeatureNameMap().at(feature));
    if (whitelist.MatchesAll())
      return Vector<String>({"*"});
    Vector<String> allowlist;
    for (const auto& origin : whitelist.Origins()) {
      allowlist.push_back(WTF::String::FromUTF8(origin.Serialize().c_str()));
    }
    return allowlist;
  }

  AddWarningForUnrecognizedFeature(feature);
  return Vector<String>();
}

void Policy::UpdateContainerPolicy(const ParsedFeaturePolicy& container_policy,
                                   const scoped_refptr<SecurityOrigin> origin) {
  policy_ = FeaturePolicy::CreateFromParentPolicy(
      document_->GetFeaturePolicy(), container_policy, origin->ToUrlOrigin());
}

void Policy::AddWarningForUnrecognizedFeature(const String& feature) const {
  document_->AddConsoleMessage(
      ConsoleMessage::Create(kOtherMessageSource, kWarningMessageLevel,
                             "Unrecognized feature: '" + feature + "'."));
}

void Policy::Trace(blink::Visitor* visitor) {
  visitor->Trace(document_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink

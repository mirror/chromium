// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/Policy.h"

#include "core/dom/Document.h"
#include "core/inspector/ConsoleMessage.h"
#include "public/platform/Platform.h"
#include "public/platform/WebSecurityOrigin.h"

namespace blink {

bool Policy::allowsFeature(const String& feature) const {
  if (feature_names_.Contains(feature))
    return policy_->IsFeatureEnabled(feature_names_.at(feature));

  AddConsoleMessageForPolicy("Unrecognized feature: '" + feature + "'.");
  return false;
}

bool Policy::allowsFeature(const String& feature, const String& origin) const {
  bool is_feature_allowed = true;
  const WebSecurityOrigin origin_ = WebSecurityOrigin::CreateFromString(origin);
  if (origin_.IsNull() || origin_.IsUnique()) {
    is_feature_allowed = false;
    AddConsoleMessageForPolicy("Invalid origin for feature '" + feature +
                               "': " + origin + ".");
  }

  if (!feature_names_.Contains(feature)) {
    is_feature_allowed = false;
    AddConsoleMessageForPolicy("Unrecognized feature: '" + feature + "'.");
  }

  return is_feature_allowed ? Platform::Current()->IsFeatureEnabledForOrigin(
                                  feature_names_.at(feature), origin_, policy_)
                            : is_feature_allowed;
}

Vector<String> Policy::allowedFeatures() const {
  Vector<String> allowed_features;
  for (const auto& entry : feature_names_) {
    if (policy_->IsFeatureEnabled(entry.value))
      allowed_features.push_back(entry.key);
  }
  return allowed_features;
}

Vector<String> Policy::getAllowlistForFeature(const String& feature) const {
  Vector<String> allowlist;
  if (feature_names_.Contains(feature)) {
    for (const auto& origin : Platform::Current()->GetOriginsForFeature(
             feature_names_.at(feature), policy_)) {
      allowlist.push_back(String(origin));
    }
  } else {
    AddConsoleMessageForPolicy("Unrecognized feature: '" + feature + "'.");
  }

  return allowlist;
}

// static
Policy* Policy::Create(Document* document) {
  return new Policy(document);
}

Policy::Policy(Document* document)
    : document_(document), policy_(document_->GetFeaturePolicy()) {}

void Policy::AddConsoleMessageForPolicy(const String& message) const {
  document_->AddConsoleMessage(ConsoleMessage::Create(
      kOtherMessageSource, kWarningMessageLevel, message));
}

DEFINE_TRACE(Policy) {
  visitor->Trace(document_);
}

}  // namespace blink

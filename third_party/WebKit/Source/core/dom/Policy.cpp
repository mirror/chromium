#include "core/dom/Policy.h"

#include "core/dom/Document.h"
#include "core/inspector/ConsoleMessage.h"

namespace blink {

bool Policy::allowsFeature(const String& feature) const {
  if (feature_names.Contains(feature)) {
    const WebFeaturePolicy* feature_policy = document_->GetFeaturePolicy();
    return feature_policy->IsFeatureEnabled(feature_names.at(feature));
  }
  document_->AddConsoleMessage(
      ConsoleMessage::Create(kOtherMessageSource, kWarningMessageLevel,
                             "Unrecognized feature: '" + feature + "'."));

  return false;
}

bool Policy::allowsFeature(const String& feature, const String& origin) const {
  if (feature_names.Contains(feature)) {
    WebSecurityOrigin origin_ = WebSecurityOrigin::CreateFromString(origin);
    if (origin_.IsNull() || origin_.IsUnique()) {
      document_->AddConsoleMessage(ConsoleMessage::Create(
          kOtherMessageSource, kWarningMessageLevel,
          "Invalid origin for feature '" + feature + "': " + origin + "."));
      return false;
    }
    const WebFeaturePolicy* feature_policy = document_->GetFeaturePolicy();
    return feature_policy->IsFeatureEnabledForOrigin(feature_names.at(feature),
                                                     origin_);
  }
  document_->AddConsoleMessage(
      ConsoleMessage::Create(kOtherMessageSource, kWarningMessageLevel,
                             "Unrecognized feature: '" + feature + "'."));

  return false;
}

Vector<String> Policy::allowedFeatures() const {
  Vector<String> allowed_features;
  const WebFeaturePolicy* policy = document_->GetFeaturePolicy();
  for (const auto& entry : feature_names) {
    if (policy->IsFeatureEnabled(entry.value))
      allowed_features.push_back(entry.key);
  }
  return allowed_features;
}

Vector<String> Policy::getAllowlist(const String& feature) const {
  Vector<String> allowlist;
  if (feature_names.Contains(feature)) {
    for (auto& origin : document_->GetFeaturePolicy()->GetOriginsForFeature(
             feature_names.at(feature))) {
      allowlist.push_back(String(origin));
    }
  } else {
    document_->AddConsoleMessage(
        ConsoleMessage::Create(kOtherMessageSource, kWarningMessageLevel,
                               "Unrecognized feature: '" + feature + "'."));
  }

  return allowlist;
}

// static
Policy* Policy::Create(Document* document) {
  return new Policy(document);
}

Policy::Policy(Document* document) : document_(document) {}

DEFINE_TRACE(Policy) {
  visitor->Trace(document_);
}

}  // namespace blink

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/origin_trials/trial_policy_impl.h"

#include "content/public/common/content_client.h"
#include "content/public/common/origin_trial_policy.h"

namespace content {

bool TrialPolicyImpl::IsValidPolicy() const {
  return policy();
}
bool TrialPolicyImpl::IsOriginTrialsFeatureEnabled() const {
  return false;
}
base::StringPiece TrialPolicyImpl::GetPublicKey() const {
  return policy()->GetPublicKey();
}
bool TrialPolicyImpl::IsFeatureDisabled(base::StringPiece feature) const {
  return policy()->IsFeatureDisabled(feature);
}
bool TrialPolicyImpl::IsTokenDisabled(base::StringPiece token_signature) const {
  return policy()->IsTokenDisabled(token_signature);
}
bool TrialPolicyImpl::IsOriginSecure(const GURL& url) const {
  return false;
}

const OriginTrialPolicy* TrialPolicyImpl::policy() const {
  return GetContentClient()->GetOriginTrialPolicy();
}

}  // namespace content

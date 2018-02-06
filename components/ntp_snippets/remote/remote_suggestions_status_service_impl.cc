// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/remote/remote_suggestions_status_service_impl.h"

#include <string>

#include "components/ntp_snippets/content_suggestions_metrics.h"
#include "components/ntp_snippets/features.h"
#include "components/ntp_snippets/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/variations/variations_associated_data.h"

namespace ntp_snippets {

RemoteSuggestionsStatusServiceImpl::RemoteSuggestionsStatusServiceImpl(
    bool is_signed_in,
    PrefService* pref_service,
    const std::string& additional_toggle_pref)
    : status_(RemoteSuggestionsStatus::EXPLICITLY_DISABLED),
      additional_toggle_pref_(additional_toggle_pref),
      is_signed_in_(is_signed_in),
      list_visible_during_session_(true),
      pref_service_(pref_service) {
  ntp_snippets::metrics::RecordRemoteSuggestionsProviderState(
      !IsExplicitlyDisabled());
}

RemoteSuggestionsStatusServiceImpl::~RemoteSuggestionsStatusServiceImpl() =
    default;

// static
void RemoteSuggestionsStatusServiceImpl::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kEnableSnippets, true);
  registry->RegisterBooleanPref(prefs::kArticlesListVisible, true);
}

void RemoteSuggestionsStatusServiceImpl::Init(
    const StatusChangeCallback& callback) {
  DCHECK(status_change_callback_.is_null());

  status_change_callback_ = callback;

  // Notify about the current state before registering the observer, to make
  // sure we don't get a double notification due to an undefined start state.
  RemoteSuggestionsStatus old_status = status_;
  status_ = GetStatusFromDeps();
  status_change_callback_.Run(old_status, status_);
  // List is never visible during session only in case it was not visible based
  // on pref setting at initialization time.
  list_visible_during_session_ =
      pref_service_->GetBoolean(prefs::kArticlesListVisible);

  pref_change_registrar_.Init(pref_service_);
  pref_change_registrar_.Add(
      prefs::kEnableSnippets,
      base::Bind(&RemoteSuggestionsStatusServiceImpl::OnSnippetsEnabledChanged,
                 base::Unretained(this)));

  if (!additional_toggle_pref_.empty()) {
    pref_change_registrar_.Add(
        additional_toggle_pref_,
        base::Bind(
            &RemoteSuggestionsStatusServiceImpl::OnSnippetsEnabledChanged,
            base::Unretained(this)));
  }
}

void RemoteSuggestionsStatusServiceImpl::OnSnippetsEnabledChanged() {
  OnStateChanged(GetStatusFromDeps());
}

void RemoteSuggestionsStatusServiceImpl::OnStateChanged(
    RemoteSuggestionsStatus new_status) {
  if (new_status == status_) {
    return;
  }

  status_change_callback_.Run(status_, new_status);
  status_ = new_status;
}

bool RemoteSuggestionsStatusServiceImpl::IsSignedIn() const {
  return is_signed_in_;
}

void RemoteSuggestionsStatusServiceImpl::OnSignInStateChanged(
    bool has_signed_in) {
  is_signed_in_ = has_signed_in;
  OnStateChanged(GetStatusFromDeps());
}

void RemoteSuggestionsStatusServiceImpl::OnListVisibilityToggled(bool visible) {
  if (visible)
    list_visible_during_session_ = true;
  pref_service_->SetBoolean(prefs::kArticlesListVisible, visible);
  OnStateChanged(GetStatusFromDeps());
}

bool RemoteSuggestionsStatusServiceImpl::IsExplicitlyDisabled() const {
  if (!pref_service_->GetBoolean(prefs::kEnableSnippets)) {
    DVLOG(1) << "[GetStatusFromDeps] Disabled via pref.";
    return true;
  }

  if (!list_visible_during_session_) {
    DVLOG(1) << "[GetStatusFromDeps] Disabled because articles list hidden.";
    return true;
  }

  // This setting will only be set if NTP Opt-out, by hidding the list is
  // disabled.
  if (!additional_toggle_pref_.empty()) {
    if (!pref_service_->GetBoolean(additional_toggle_pref_)) {
      DVLOG(1) << "[GetStatusFromDeps] Disabled via additional pref";
      return true;
    }
  }

  return false;
}

RemoteSuggestionsStatus RemoteSuggestionsStatusServiceImpl::GetStatusFromDeps()
    const {
  if (IsExplicitlyDisabled()) {
    return RemoteSuggestionsStatus::EXPLICITLY_DISABLED;
  }

  DVLOG(1) << "[GetStatusFromDeps] Enabled, signed "
           << (IsSignedIn() ? "in" : "out");
  return IsSignedIn() ? RemoteSuggestionsStatus::ENABLED_AND_SIGNED_IN
                      : RemoteSuggestionsStatus::ENABLED_AND_SIGNED_OUT;
}

}  // namespace ntp_snippets

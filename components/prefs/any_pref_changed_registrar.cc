// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/prefs/any_pref_changed_registrar.h"

#include "components/prefs/pref_service.h"

AnyPrefChangedRegistrar::AnyPrefChangedRegistrar() = default;
AnyPrefChangedRegistrar::~AnyPrefChangedRegistrar() {
  if (service_)
    Unregister();
}

void AnyPrefChangedRegistrar::Register(PrefService* service,
                                       const NamedChangeCallback& obs) {
  DCHECK(!service_);
  service_ = service;
  callback_ = obs;
}

void AnyPrefChangedRegistrar::Unregister() {
  service_->RemoveAnyPrefObserver(this);
  service_ = nullptr;
}

void AnyPrefChangedRegistrar::OnPreferenceChanged(
    PrefService* service,
    const std::string& pref_name) {
  callback_.Run(pref_name);
}

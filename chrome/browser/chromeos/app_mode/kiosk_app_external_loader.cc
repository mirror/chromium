// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/app_mode/kiosk_app_external_loader.h"

#include <utility>

#include "base/values.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_manager.h"

namespace chromeos {

KioskAppExternalLoader::KioskAppExternalLoader(AppClass app_class)
    : app_class_(app_class), kiosk_app_manager_observer_(this) {}

KioskAppExternalLoader::~KioskAppExternalLoader() = default;

void KioskAppExternalLoader::StartLoading() {
  if (state_ != State::INITIAL)
    return;

  state_ = State::LOADING;
  kiosk_app_manager_observer_.Add(KioskAppManager::Get());

  SendPrefsIfAvailable();
}

void KioskAppExternalLoader::OnPrimaryKioskAppReadyToLoad() {
  if (app_class_ == AppClass::PRIMARY)
    SendPrefsIfAvailable();
}

void KioskAppExternalLoader::OnSecondaryKioskAppsReadyToLoad() {
  if (app_class_ == AppClass::SECONDARY)
    SendPrefsIfAvailable();
}

std::unique_ptr<base::DictionaryValue> KioskAppExternalLoader::GetAppsPrefs() {
  switch (app_class_) {
    case AppClass::PRIMARY:
      return KioskAppManager::Get()->GetPrimaryAppPrefs();
    case AppClass::SECONDARY:
      return KioskAppManager::Get()->GetSecondaryAppsPrefs();
  }
  return nullptr;
}

void KioskAppExternalLoader::SendPrefsIfAvailable() {
  std::unique_ptr<base::DictionaryValue> prefs = GetAppsPrefs();
  if (!prefs)
    return;

  const bool initial_load = state_ == State::LOADING;
  state_ = State::LOADED;

  if (prefs_interceptor_) {
    prefs_interceptor_.Run(initial_load, std::move(prefs));
    return;
  }

  if (initial_load) {
    LoadFinished(std::move(prefs));
  } else {
    OnUpdated(std::move(prefs));
  }
}

}  // namespace chromeos

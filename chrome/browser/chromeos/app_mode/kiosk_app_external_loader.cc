// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/app_mode/kiosk_app_external_loader.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/values.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_manager.h"

namespace chromeos {

KioskAppExternalLoader::KioskAppExternalLoader(AppClass app_class)
    : app_class_(app_class) {}

KioskAppExternalLoader::~KioskAppExternalLoader() {
  if (state_ != State::INITIAL) {
    switch (app_class_) {
      case AppClass::PRIMARY:
        KioskAppManager::Get()->SetPrimaryAppReadyHandler(
            base::RepeatingClosure());
        break;
      case AppClass::SECONDARY:
        KioskAppManager::Get()->SetSecondaryAppsReadyHandler(
            base::RepeatingClosure());
        break;
    }
  }
}

void KioskAppExternalLoader::StartLoading() {
  if (state_ != State::INITIAL)
    return;

  state_ = State::LOADING;

  base::RepeatingClosure handler =
      base::BindRepeating(&KioskAppExternalLoader::SendPrefsIfAvailable,
                          weak_ptr_factory_.GetWeakPtr());
  switch (app_class_) {
    case AppClass::PRIMARY:
      KioskAppManager::Get()->SetPrimaryAppReadyHandler(std::move(handler));
      break;
    case AppClass::SECONDARY:
      KioskAppManager::Get()->SetSecondaryAppsReadyHandler(std::move(handler));
      break;
  }

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

  if (initial_load) {
    LoadFinished(std::move(prefs));
  } else {
    OnUpdated(std::move(prefs));
  }
}

}  // namespace chromeos

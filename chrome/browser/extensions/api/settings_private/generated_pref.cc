// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/settings_private/generated_pref.h"

#include "base/callback.h"

namespace extensions {
namespace settings_private {

GeneratedPref::GeneratedPref() = default;
GeneratedPref::~GeneratedPref() = default;

void GeneratedPref::AddPrefObserver(const PrefChangeCallback& observer) {
  observers_.push_back(observer);
}

void GeneratedPref::NotifyObservers(const std::string& pref_name) {
  for (const auto& observer : observers_)
    observer.Run(pref_name);
}

}  // namespace settings_private
}  // namespace extensions

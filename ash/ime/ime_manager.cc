// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/ime/ime_manager.h"

namespace ash {

ImeManager::ImeManager() = default;

ImeManager::~ImeManager() = default;

IMEInfo ImeManager::GetCurrentIme() {
  return IMEInfo();
}

std::vector<IMEPropertyInfo> ImeManager::GetCurrentImeProperties() {
  return std::vector<IMEPropertyInfo>();
}

std::vector<IMEInfo> ImeManager::GetAvailableImes() {
  return std::vector<IMEInfo>();
}

bool ImeManager::IsImeManaged() {
  return false;
}

}  // namespace ash

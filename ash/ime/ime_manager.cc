// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/ime/ime_manager.h"

#include "ash/public/interfaces/ime_info.mojom.h"

namespace ash {

ImeManager::ImeManager() = default;

ImeManager::~ImeManager() = default;

mojom::ImeInfo ImeManager::GetCurrentIme() const {
  return mojom::ImeInfo();
}

std::vector<mojom::ImeProperty> ImeManager::GetCurrentImeProperties() const {
  return std::vector<mojom::ImeProperty>();
}

std::vector<mojom::ImeInfo> ImeManager::GetAvailableImes() const {
  return std::vector<mojom::ImeInfo>();
}

bool ImeManager::IsImeManaged() const {
  return false;
}

}  // namespace ash

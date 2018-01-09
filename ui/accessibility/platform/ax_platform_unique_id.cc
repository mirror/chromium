// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "ui/accessibility/platform/ax_platform_unique_id.h"

namespace ui {

AXUniqueId::AXUniqueId(const int32_t max_id) : id_(GetNextAXUniqueId(max_id)) {}

AXUniqueId::~AXUniqueId() {
  AssignedIds()->erase(id_);
}

base::hash_set<int32_t>* AXUniqueId::AssignedIds() {
  static base::hash_set<int32_t>* assigned_ids;
  if (!assigned_ids)
    assigned_ids = new base::hash_set<int32_t>();
  return assigned_ids;
}

bool AXUniqueId::IsAssigned(int32_t id) const {
  return AssignedIds()->find(id) != AssignedIds()->end();
}

int32_t AXUniqueId::GetNextAXUniqueId(const int32_t max_id) {
  static int32_t current_unique_id = 0;
  static bool has_wrapped = false;

  while (true) {
    if (current_unique_id == max_id) {
      current_unique_id = 1;
      has_wrapped = true;
    } else {
      current_unique_id++;
    }
    if (!has_wrapped || !IsAssigned(current_unique_id))
      break;
  }

  AssignedIds()->insert(current_unique_id);

  return current_unique_id;
}

}  // namespace ui

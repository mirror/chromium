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
  static int32_t current_id = 0;
  static bool has_wrapped = false;

  const int32_t prev_id = current_id;

  while (true) {
    if (current_id == max_id) {
      current_id = 1;
      has_wrapped = true;
    } else {
      current_id++;
    }
    if (!has_wrapped || !IsAssigned(current_id) || current_id == prev_id)
      break;
  }

  AssignedIds()->insert(current_id);

  return current_id;
}

}  // namespace ui

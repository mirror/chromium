// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_PLATFORM_AX_PLATFORM_UNIQUE_ID_H_
#define UI_ACCESSIBILITY_PLATFORM_AX_PLATFORM_UNIQUE_ID_H_

#include <stdint.h>

#include "base/containers/hash_tables.h"
#include "ui/accessibility/ax_export.h"

namespace ui {

// Each platform accessibility object has a unique id that's guaranteed to be a
// positive number. (It's stored in an int32_t as opposed to uint32_t because
// some platforms want to negate it, so we want to ensure the range is below the
// signed int max.) This can be used when the id has to be unique across
// multiple frames, since node ids are only unique within one tree.
// In addition to generating a unique id, this class also helps ensure that
// programmers don't accidentally conflate these unique ids with the int id
// that comes with web node data.

class AX_EXPORT AXUniqueId {
 public:
  AXUniqueId();
  ~AXUniqueId();

  int32_t Get() const { return id_; }

  bool operator==(const AXUniqueId& other) const {
    return Get() == other.Get();
  }

  bool operator!=(const AXUniqueId& other) const {
    return Get() != other.Get();
  }

 private:
  int32_t GetNextAXUniqueId();

  bool IsAssigned(int32_t) const;

  int32_t id_;

  static base::hash_set<int32_t>* AssignedIds();

  DISALLOW_COPY_AND_ASSIGN(AXUniqueId);
};

}  // namespace ui

#endif  // UI_ACCESSIBILITY_PLATFORM_AX_PLATFORM_UNIQUE_ID_H_

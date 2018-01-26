// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/variant.h"

namespace base {

AbstractVariant::~AbstractVariant() {
  type_ops_[index_].destruct(&union_[0]);
}

AbstractVariant::AbstractVariant(AbstractVariant&& other) {
  type_ops_ = other.type_ops_;
  num_types_ = other.num_types_;
  index_ = other.index_;
  union_ = std::move(other.union_);
}

}  // namespace base

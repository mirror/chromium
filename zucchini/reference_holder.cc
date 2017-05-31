// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/reference_holder.h"

#include <functional>

namespace zucchini {

const ReferenceHolder::LastTag ReferenceHolder::kLast = {};

ReferenceHolder::ReferenceHolder() = default;

ReferenceHolder::ReferenceHolder(ReferenceHolder&) = default;

ReferenceHolder::ReferenceHolder(uint8_t type_count)
    : references_(type_count), traits_(type_count) {}

ReferenceHolder::~ReferenceHolder() = default;

// static
const ReferenceHolder& ReferenceHolder::GetEmptyRefs() {
  static ReferenceHolder empty_refs;
  return empty_refs;
}

void ReferenceHolder::Insert(GroupRange groups) {
  for (ReferenceGroup group : groups) {
    Insert(group);
  }
}

void ReferenceHolder::Insert(const ReferenceGroup& refs) {
  Insert(refs.Traits(), ranges::view::MakeIterable<Reference>(refs.Find()));
}

Reference ReferenceHolder::Find(uint8_t type, offset_t upper) const {
  auto it = ranges::upper_bound(references_[type], upper, std::less<offset_t>(),
                                &Reference::location);
  if (it != references_[type].begin()) {
    return *(--it);
  }
  return {};
}

}  // namespace zucchini

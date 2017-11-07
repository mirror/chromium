// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// NOTE: This file is only compiled when Crashpad is not used as the crash
// reproter.

#include "components/crash/core/common/crash_key.h"

namespace crash_reporter {
namespace internal {

TransitionalCrashKeyStorage* GetCrashKeyStorage() {
  static TransitionalCrashKeyStorage* storage =
      new TransitionalCrashKeyStorage();
  return storage;
}

void CrashKeyStringImpl::Set(const char* value) {
  if (is_set()) {
    GetCrashKeyStorage()->SetValueAtIndex(index_, value);
  } else {
    index_ = GetCrashKeyStorage()->SetKeyValue(name_, value);
  }
}

void CrashKeyStringImpl::Clear() {
  GetCrashKeyStorage()->RemoveAtIndex(index_);
  index_ = TransitionalCrashKeyStorage::num_entries;
}

bool CrashKeyStringImpl::is_set() const {
  return index_ != TransitionalCrashKeyStorage::num_entries;
}

}  // namespace internal
}  // namespace crash_reporter

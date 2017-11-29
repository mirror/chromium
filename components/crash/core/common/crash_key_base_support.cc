// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/core/common/crash_key_base_support.h"

#include "base/debug/crash_logging.h"
#include "components/crash/core/common/crash_key.h"

namespace crash_reporter {

namespace {

// This stores the value for a crash key allocated through the //base API.
template <uint32_t ValueSize>
struct BaseCrashKeyString : public base::debug::CrashKeyString {
  BaseCrashKeyString(const char name[], base::debug::CrashKeySize size)
      : base::debug::CrashKeyString(name, size), impl(name) {
    DCHECK_EQ(static_cast<uint32_t>(size), ValueSize);
  }
  crash_reporter::CrashKeyString<ValueSize> impl;
};

#define SIZE_CLASS_OPERATION(size_class, operation_prefix, operation_suffix) \
  switch (size_class) {                                                      \
    case base::debug::CrashKeySize::Size32:                                  \
      operation_prefix BaseCrashKeyString<32> operation_suffix;              \
    case base::debug::CrashKeySize::Size64:                                  \
      operation_prefix BaseCrashKeyString<64> operation_suffix;              \
    case base::debug::CrashKeySize::Size256:                                 \
      operation_prefix BaseCrashKeyString<256> operation_suffix;             \
  }

}  // namespace

void InitializeCrashKeyBaseSupport() {
  base::debug::CrashKeyImplementation impl{};
  impl.allocate =
      [](const char name[],
         base::debug::CrashKeySize size) -> base::debug::CrashKeyString* {
    SIZE_CLASS_OPERATION(size, return new, (name, size));
    return nullptr;
  };
  impl.set = [](base::debug::CrashKeyString* crash_key,
                base::StringPiece value) {
    SIZE_CLASS_OPERATION(crash_key->size,
                         reinterpret_cast<, *>(crash_key)->impl.Set(value));
  };
  impl.clear = [](base::debug::CrashKeyString* crash_key) {
    SIZE_CLASS_OPERATION(crash_key->size,
                         reinterpret_cast<, *>(crash_key)->impl.Clear());
  };

  base::debug::SetCrashKeyImplementation(&impl);
}

}  // namespace crash_reporter

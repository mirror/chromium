// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/core/common/crash_key_base_support.h"

#include <memory>

#include "base/debug/crash_logging.h"
#include "base/debug/leak_annotations.h"
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
      break;                                                                 \
    case base::debug::CrashKeySize::Size64:                                  \
      operation_prefix BaseCrashKeyString<64> operation_suffix;              \
      break;                                                                 \
    case base::debug::CrashKeySize::Size256:                                 \
      operation_prefix BaseCrashKeyString<256> operation_suffix;             \
      break;                                                                 \
  }

class CrashKeyBaseSupport : public base::debug::CrashKeyImplementation {
 public:
  CrashKeyBaseSupport() = default;

  ~CrashKeyBaseSupport() override = default;

  base::debug::CrashKeyString* Allocate(
      const char name[],
      base::debug::CrashKeySize size) override {
    base::debug::CrashKeyString* crash_key = nullptr;

    SIZE_CLASS_OPERATION(size, crash_key = new, (name, size));

    // Crash keys allocated through this API are never freed.
    if (crash_key)
      ANNOTATE_LEAKING_OBJECT_PTR(crash_key);
    return crash_key;
  }

  void Set(base::debug::CrashKeyString* crash_key,
           base::StringPiece value) override {
    SIZE_CLASS_OPERATION(crash_key->size,
                         reinterpret_cast<, *>(crash_key)->impl.Set(value));
  }

  void Clear(base::debug::CrashKeyString* crash_key) override {
    SIZE_CLASS_OPERATION(crash_key->size,
                         reinterpret_cast<, *>(crash_key)->impl.Clear());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CrashKeyBaseSupport);
};

#undef SIZE_CLASS_OPERATION

}  // namespace

void InitializeCrashKeyBaseSupport() {
  base::debug::SetCrashKeyImplementation(
      std::make_unique<CrashKeyBaseSupport>());
}

}  // namespace crash_reporter

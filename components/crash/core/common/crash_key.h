// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRASH_CORE_COMMON_CRASH_KEY_H_
#define COMPONENTS_CRASH_CORE_COMMON_CRASH_KEY_H_

#include "base/macros.h"
#include "build/build_config.h"

// The crash key interface exposed by this file is the same as the Crashpad
// Annotation interface. Because not all platforms use Crashpad yet, a
// source-compatible interface is provided on top of the older Breakpad
// storage mechanism.
#if defined(OS_MACOSX) || defined(OS_WIN)
#define USE_CRASHPAD_ANNOTATION 1
#endif

#if defined(USE_CRASHPAD_ANNOTATION)
#include "third_party/crashpad/crashpad/client/annotation.h"
#else
#include "third_party/breakpad/breakpad/src/common/simple_string_dictionary.h"
#endif

namespace crash_reporter {

// A CrashKeyString stores a name-value pair that will be recorded within a
// crash report.
//
// The crash key name must be a constant string expression, and the value
// should be unique and identifying. The maximum size for the value is
// specified as the template argument, and values greater than this are
// truncated. Crash keys should be declared with static storage duration.
//
// Example:
//    namespace {
//    crash_reporter::CrashKeyString<256> g_active_url("current-page-url");
//    }
//
//    void DidNaviagate(GURL new_url) {
//      g_active_url.Set(new_url.ToString());
//    }
#if defined(USE_CRASHPAD_ANNOTATION)

template <crashpad::Annotation::ValueSizeType MaxLength>
using CrashKeyString = crashpad::StringAnnotation<MaxLength>;

#else  // Crashpad-compatible crash key interface:

namespace internal {

// Accesses the underlying storage for crash keys for non-Crashpad clients.
using TransitionalCrashKeyStorage =
    google_breakpad::NonAllocatingMap<32, 2048, 64>;
TransitionalCrashKeyStorage* GetCrashKeyStorage();

// Base implementation of a CrashKeyString for non-Crashpad clients. A separate
// base class is necessary so that ScopedClearCrashKey can refer to an instance
// without template arguments.
class CrashKeyStringImpl {
 public:
  constexpr explicit CrashKeyStringImpl(const char name[])
      : name_(name), index_(TransitionalCrashKeyStorage::num_entries) {}

  void Set(const char* value);
  void Clear();

  bool is_set() const;

 private:
  // The name of the crash key.
  const char* const name_;

  // If the crash key is set, this is the index into the storage that can be
  // used to set/clear the key without requiring a linear scan of the storage
  // table. This will be |num_entries| if unset.
  size_t index_;
};

}  // namespace internal

template <size_t MaxLength_Unused>
class CrashKeyString : public internal::CrashKeyStringImpl {
 public:
  using CrashKeyStringImpl::CrashKeyStringImpl;

 private:
  DISALLOW_COPY_AND_ASSIGN(CrashKeyString);
};

#endif

// This scoper clears the specified annotation's value when it goes out of
// scope.
//
// Example:
//    void DoSomething(const std::string& data) {
//      static crash_reporter::CrashKeyString<32> crash_key("DoSomething-data");
//      crash_reporter::ScopedClearCrashKey auto_clear(&crash_key);
//      crash_key.Set(data);
//
//      DoSomethignImpl(data);
//    }
class ScopedClearCrashKey {
 public:
#if defined(USE_CRASHPAD_ANNOTATION)
  using CrashKeyType = crashpad::Annotation*;
#else
  using CrashKeyType = internal::CrashKeyStringImpl*;
#endif

  explicit ScopedClearCrashKey(CrashKeyType crash_key)
      : crash_key_(crash_key) {}

  ~ScopedClearCrashKey() { crash_key_->Clear(); }

 private:
  CrashKeyType const crash_key_;
  DISALLOW_COPY_AND_ASSIGN(ScopedClearCrashKey);
};

}  // namespace crash_reporter

#undef USE_CRASHPAD_ANNOTATION

#endif  // COMPONENTS_CRASH_CORE_COMMON_CRASH_KEY_H_

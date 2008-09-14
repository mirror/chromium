// Copyright 2006-2008 Google, Inc. All Rights Reserved.
// <<license>>

//
// Top include for all V8 .cc files.
//

#ifndef V8_V8_H_
#define V8_V8_H_

// Google3 uses NDEBUG.
#if defined(GOOGLE3) && !defined(NDEBUG)
#define DEBUG
#endif

// V8 only uses DEBUG, but included external files
// may use NDEBUG - make sure they are consistent.
#if defined(DEBUG) && defined(NDEBUG)
#error both DEBUG and NDEBUG are set
#endif

// Basic includes
#include "../public/v8.h"
#include "globals.h"
#include "checks.h"
#include "allocation.h"
#include "utils.h"
#include "flags-inl.h"

// Objects & heap
#include "objects.h"
#include "spaces.h"
#include "heap.h"
#include "objects-inl.h"
#include "spaces-inl.h"
#include "heap-inl.h"
#include "messages.h"

namespace v8 { namespace internal {

class V8 : public AllStatic {
 public:
  // Global actions.

  // If Initialize is called with des == NULL, the
  // initial state is created from scratch. If a non-null Deserializer
  // is given, the initial state is created by reading the
  // deserialized data into an empty heap.
  static bool Initialize(Deserializer* des);
  static void TearDown();
  static bool HasBeenSetup() { return has_been_setup_; }
  static bool HasBeenDisposed() { return has_been_disposed_; }

  // Report process out of memory. Implementation found in api.cc.
  static void FatalProcessOutOfMemory(const char* location);
 private:
  static bool has_been_setup_;
  static bool has_been_disposed_;
};

} }  // namespace v8::internal

#endif  // V8_V8_H_

// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_COMPONENT_EXPORT_H_
#define BASE_COMPONENT_EXPORT_H_

#include "build/build_config.h"

// Used to annotate symbols which are exported by the component named
// |component|. Note that this only does the right thing if the corresponding
// component target's sources are compiled with |IS_$component_IMPL| defined
// as 1. For example:
//
//   class COMPONENT_EXPORT(FOO) Bar {};
//
// If IS_FOO_IMPL=1 at compile time, then Bar will be annotated using the
// COMPONENT_EXPORT_ANNOTATION macro defined below. Otherwise it will be
// annotated using the COMPONENT_IMPORT_ANNOTATION macro.
#define COMPONENT_EXPORT(name)                              \
  COMPONENT_MACRO_CONDITIONAL_(INSIDE_COMPONENT_IMPL(name), \
                               COMPONENT_EXPORT_ANNOTATION, \
                               COMPONENT_IMPORT_ANNOTATION)

// Indicates whether the current compilation unit is being compiled as part of
// the implementation of the component named |component|. Expands to |1| if
// |IS_$component_IMPL| is defined as |1|; expands to |0| otherwise.
#define INSIDE_COMPONENT_IMPL_(component) \
  COMPONENT_MACRO_IS_1_(IS_##component##_IMPL)

// Compiler-specific macros to annotate for export or import of a symbol. No-op
// in non-component builds.
//
// These should not see much if any direct use. Instead, use the
// COMPONENT_EXPORT macro defined above.
#if defined(COMPONENT_BUILD)
#if defined(WIN32)
#define COMPONENT_EXPORT_ANNOTATION __declspec(dllexport)
#define COMPONENT_IMPORT_ANNOTATION __declspec(dllimport)
#else  // defined(WIN32)
#define COMPONENT_EXPORT_ANNOTATION __attribute__((visibility("default")))
#define COMPONENT_IMPORT_ANNOTATION
#endif  // defined(WIN32)
#else   // defined(COMPONENT_BUILD)
#define COMPONENT_EXPORT_ANNOTATION
#define COMPONENT_IMPORT_ANNOTATION
#endif  // defined(COMPONENT_BUILD)

// Below this point are several internal utility macros used for the
// implementation of the above macros. Not intended for external use.

// Helper for COMPONENT_EXPORT to conditionally expand to either export or
// import. If |condition| expands to |1| then this macro evaluates to
// |consequent|; otherwise it evaluates to |alternate|.
#define COMPONENT_MACRO_CONDITIONAL_(condition, consequent, alternate)        \
  COMPONENT_MACRO_CONDITIONAL_HELPER_(COMPONENT_MACRO_TEST_VALUE_(condition), \
                                      consequent, alternate)
#define COMPONENT_MACRO_CONDITIONAL_HELPER_(...) \
  COMPONENT_MACRO_CONDITIONAL_HELPER_2_(__VA_ARGS__)
#define COMPONENT_MACRO_CONDITIONAL_HELPER_2_(a, b, c, ...) c

// Helper to determine whether a given argument expands to |1|. This expands to
// |1| if so, otherwise it expands to |0|.
#define COMPONENT_MACRO_IS_1_(a) \
  COMPONENT_MACRO_TEST_(COMPONENT_MACRO_TEST_VALUE_(a))

// Concatenates the first and second arguments, expanded if necessary.
#define COMPONENT_MACRO_CONCAT_(a, ...) a##__VA_ARGS__

// Helper to test the value of a thing. This always expands to its *second*
// argument if given; otherwise expands to |0|. Used in conjunciton with
// COMPONENT_MACRO_TEST_VALUE_(). Examples:
//
//   COMPONENT_MACRO_TEST_() => 0
//   COMPONENT_MACRO_TEST_(1) => 0
//   COMPONENT_MACRO_TEST_(a) => 0
//   COMPONENT_MACRO_TEST_(a, b) => b
//   COMPONENT_MACRO_TEST_(a, b, c, d, e) => b
#define COMPONENT_MACRO_TEST_(...) COMPONENT_MACRO_TEST_IMPL_(__VA_ARGS__, 0)
#define COMPONENT_MACRO_TEST_IMPL_(a, b, ...) b

// Helper used in conjunction with COMPONENT_MACRO_CONDITIONAL_ and
// COMPONENT_MACRO_IS_1_ above. Turns a "true" value (i.e. a |1|) into
// a sequence of tokens |$IGNORED$, 1|, and turns any other value into
// an arbitrary (unimportant) single token. Thus it can be used to induce
// the aforementioned macros to select a different positional argument.
#define COMPONENT_MACRO_TEST_VALUE_(a)     \
  COMPONENT_MACRO_MAKE_TEST_VALUE_RESULT_( \
      COMPONENT_MACRO_CONCAT_(COMPONENT_MACRO_TEST_VALUE_RESULT_, a))
#define COMPONENT_MACRO_MAKE_TEST_VALUE_RESULT_(...) \
  COMPONENT_MACRO_CONCAT_(__VA_ARGS__, _)
#define COMPONENT_MACRO_TEST_VALUE_RESULT_1_ $IGNORED$, 1

#endif  // BASE_COMPONENT_EXPORT_H_

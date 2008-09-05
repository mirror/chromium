// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#include <stdarg.h>

#include "v8.h"

#include "platform.h"
#include "top.h"

using namespace v8::internal;

DEFINE_bool(stack_trace_on_abort, true,
            "print a stack trace if an assertion failure occurs");

#ifdef DEBUG
DEFINE_bool(enable_slow_asserts, false,
            "enable asserts that are slow to execute");
#endif

static int fatal_error_handler_nesting_depth = 0;

// Contains protection against recursive calls (faults while handling faults).
extern "C" void V8_Fatal(const char* file, int line, const char* format, ...) {
  fatal_error_handler_nesting_depth++;
  // First time we try to print an error message
  if (fatal_error_handler_nesting_depth < 2) {
    OS::PrintError("\n\n#\n# Fatal error in %s, line %d\n# ", file, line);
    va_list arguments;
    va_start(arguments, format);
    OS::VPrintError(format, arguments);
    va_end(arguments);
    OS::PrintError("\n#\n\n");
  }
  // First two times we may try to print a stack dump.
  if (fatal_error_handler_nesting_depth < 3) {
    if (FLAG_stack_trace_on_abort) {
      // Call this one twice on double fault
      Top::PrintStack();
    }
  }
  OS::Abort();
}


void CheckEqualsHelper(const char* file,
                       int line,
                       const char* expected_source,
                       v8::Handle<v8::Value> expected,
                       const char* value_source,
                       v8::Handle<v8::Value> value) {
  if (!expected->Equals(value)) {
    v8::String::AsciiValue value_str(value);
    v8::String::AsciiValue expected_str(expected);
    V8_Fatal(file, line,
             "CHECK_EQ(%s, %s) failed\n#   Expected: %s\n#   Found: %s",
             expected_source, value_source, *expected_str, *value_str);
  }
}


void CheckNonEqualsHelper(const char* file,
                          int line,
                          const char* unexpected_source,
                          v8::Handle<v8::Value> unexpected,
                          const char* value_source,
                          v8::Handle<v8::Value> value) {
  if (unexpected->Equals(value)) {
    v8::String::AsciiValue value_str(value);
    V8_Fatal(file, line, "CHECK_NE(%s, %s) failed\n#   Value: %s",
             unexpected_source, value_source, *value_str);
  }
}


void API_Fatal(const char* location, const char* format, ...) {
  OS::PrintError("\n#\n# Fatal error in %s\n# ", location);
  va_list arguments;
  va_start(arguments, format);
  OS::VPrintError(format, arguments);
  va_end(arguments);
  OS::PrintError("\n#\n\n");
  OS::Abort();
}

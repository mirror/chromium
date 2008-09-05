// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>
//
// A simple interactive shell.  Enable with --shell.

#include "../public/debug.h"

namespace v8 { namespace internal {

// Debug event handler for interactive debugging.
void handle_debug_event(v8::DebugEvent event,
                        v8::Handle<v8::Object> exec_state,
                        v8::Handle<v8::Object> event_data,
                        v8::Handle<Value> data);


class Shell {
 public:
  static void PrintObject(v8::Handle<v8::Value> obj);
  // Run the read-eval loop, executing code in the specified
  // environment.
  static void Run(v8::Handle<v8::Context> context);
};

} }  // namespace v8::internal

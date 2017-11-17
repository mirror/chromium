// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_BINDINGS_JS_RUNNER_H_
#define EXTENSIONS_RENDERER_BINDINGS_JS_RUNNER_H_

#include "v8/include/v8.h"

namespace extensions {

// A helper class to execute JS functions safely.
class JSRunner {
 public:
  // Returns the instance of the JSRunner for the current thread.
  static JSRunner* Get();

  // Sets the instance of the JSRunner for the current thread. Does *not* take
  // ownership.
  static void SetThreadInstance(JSRunner* js_runner);

  virtual ~JSRunner() {}

  // Calls the given |function| in the specified |context| and with the provided
  // arguments. JS may be executed asynchronously if it has been suspended in
  // the context.
  virtual void RunJSFunction(v8::Local<v8::Function> function,
                             v8::Local<v8::Context> context,
                             int argc,
                             v8::Local<v8::Value> argv[]) = 0;

  // Executes the given |function| synchronously and returns the result. Note
  // this should only be called if you're certain that script is not currently
  // suspended in this context; if execution is suspended, this will fail a
  // CHECK. We use a Global instead of a Local because certain implementations
  // need to create a persistent handle in order to prevent immediate
  // destruction of the locals.
  // TODO(devlin): if we could, using Local here with an EscapableHandleScope
  // would be preferable.
  virtual v8::Global<v8::Value> RunJSFunctionSync(
      v8::Local<v8::Function> function,
      v8::Local<v8::Context> context,
      int argc,
      v8::Local<v8::Value> argv[]) = 0;
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_BINDINGS_JS_RUNNER_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/extension_js_runner.h"

#include "content/public/renderer/worker_thread.h"
#include "extensions/renderer/script_context.h"

namespace extensions {

ExtensionJSRunner::ExtensionJSRunner(ScriptContext* script_context)
    : script_context_(script_context) {}
ExtensionJSRunner::~ExtensionJSRunner() {}

void ExtensionJSRunner::RunJSFunction(v8::Local<v8::Function> function,
                                      v8::Local<v8::Context> context,
                                      int argc,
                                      v8::Local<v8::Value> argv[]) {
  RunJSFunction(function, context, argc, argv, ResultCallback());
}

void ExtensionJSRunner::RunJSFunction(v8::Local<v8::Function> function,
                                      v8::Local<v8::Context> context,
                                      int argc,
                                      v8::Local<v8::Value> argv[],
                                      ResultCallback callback) {
  // TODO(devlin): Move ScriptContext::SafeCallFunction() into here?
  script_context_->SafeCallFunction(function, argc, argv, std::move(callback));
}

v8::Global<v8::Value> ExtensionJSRunner::RunJSFunctionSync(
    v8::Local<v8::Function> function,
    v8::Local<v8::Context> context,
    int argc,
    v8::Local<v8::Value> argv[]) {
  DCHECK(script_context_->v8_context() == context);

  bool did_complete = false;
  v8::Global<v8::Value> result;
  auto callback = base::Bind(
      [](bool* did_complete_out, v8::Global<v8::Value>* result_out,
         v8::Local<v8::Context> context, v8::Local<v8::Value> result) {
        *did_complete_out = true;
        // The locals are released after the callback is executed, so we need to
        // grab a persistent handle.
        if (!result.IsEmpty())
          result_out->Reset(context->GetIsolate(), result);
      },
      base::Unretained(&did_complete), base::Unretained(&result));

  script_context_->SafeCallFunction(function, argc, argv, callback);
  CHECK(did_complete) << "expected script to execute synchronously";
  return result;
}

}  // namespace extensions

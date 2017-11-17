// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/extension_js_runner.h"

#include "content/public/renderer/worker_thread.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/script_context_set.h"
#include "extensions/renderer/worker_thread_dispatcher.h"

namespace extensions {

namespace {

ScriptContext* GetScriptContext(v8::Local<v8::Context> context) {
  ScriptContext* script_context =
      content::WorkerThread::GetCurrentId() > 0
          ? WorkerThreadDispatcher::GetScriptContext()
          : ScriptContextSet::GetContextByV8Context(context);

  // TODO(devlin): Is this CHECK safe? If our custom bindings can continue
  // running after contexts are removed from the set, it's likely not, but
  // that's mostly a bug. In a perfect world, we *should* be able to do this
  // CHECK.
  CHECK(script_context);

  DCHECK(script_context->v8_context() == context);
  return script_context;
}

}  // namespace

ExtensionJSRunner::ExtensionJSRunner() {
  JSRunner::SetThreadInstance(this);
}

ExtensionJSRunner::~ExtensionJSRunner() {
  JSRunner::SetThreadInstance(nullptr);
}

void ExtensionJSRunner::RunJSFunction(v8::Local<v8::Function> function,
                                      v8::Local<v8::Context> context,
                                      int argc,
                                      v8::Local<v8::Value> argv[]) {
  // TODO(devlin): Move ScriptContext::SafeCallFunction() into here?
  ScriptContext* script_context = GetScriptContext(context);
  script_context->SafeCallFunction(function, argc, argv);
}

v8::Global<v8::Value> ExtensionJSRunner::RunJSFunctionSync(
    v8::Local<v8::Function> function,
    v8::Local<v8::Context> context,
    int argc,
    v8::Local<v8::Value> argv[]) {
  bool did_complete = false;
  v8::Global<v8::Value> result;
  auto callback = base::Bind(
      [](v8::Isolate* isolate, bool* did_complete_out,
         v8::Global<v8::Value>* result_out,
         const std::vector<v8::Local<v8::Value>>& results) {
        *did_complete_out = true;
        // The locals are released after the callback is executed, so we need to
        // grab a persistent handle.
        if (!results.empty() && !results[0].IsEmpty())
          result_out->Reset(isolate, results[0]);
      },
      base::Unretained(context->GetIsolate()), base::Unretained(&did_complete),
      base::Unretained(&result));

  ScriptContext* script_context = GetScriptContext(context);
  script_context->SafeCallFunction(function, argc, argv, callback);
  CHECK(did_complete) << "expected script to execute synchronously";
  return result;
}

}  // namespace extensions

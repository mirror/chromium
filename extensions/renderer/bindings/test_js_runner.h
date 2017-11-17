// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_BINDINGS_TEST_JS_RUNNER_H_
#define EXTENSIONS_RENDERER_BINDINGS_TEST_JS_RUNNER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "extensions/renderer/bindings/js_runner.h"

namespace extensions {

class TestJSRunner : public JSRunner {
 public:
  class Scope {
   public:
    Scope(std::unique_ptr<JSRunner> runner);
    ~Scope();

   private:
    std::unique_ptr<JSRunner> runner_;
    JSRunner* old_runner_;

    DISALLOW_COPY_AND_ASSIGN(Scope);
  };

  class AllowErrors {
   public:
    AllowErrors();
    ~AllowErrors();

   private:
    DISALLOW_COPY_AND_ASSIGN(AllowErrors);
  };

  TestJSRunner();
  TestJSRunner(const base::Closure& did_call_js);
  ~TestJSRunner() override;

  // JSRunner:
  void RunJSFunction(v8::Local<v8::Function> function,
                     v8::Local<v8::Context> context,
                     int argc,
                     v8::Local<v8::Value> argv[]) override;
  v8::Global<v8::Value> RunJSFunctionSync(v8::Local<v8::Function> function,
                                          v8::Local<v8::Context> context,
                                          int argc,
                                          v8::Local<v8::Value> argv[]) override;

 private:
  base::Closure did_call_js_;

  DISALLOW_COPY_AND_ASSIGN(TestJSRunner);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_BINDINGS_TEST_JS_RUNNER_H_

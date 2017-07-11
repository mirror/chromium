// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_BINDINGS_EXCEPTION_HANDLER
#define EXTENSIONS_RENDERER_BINDINGS_EXCEPTION_HANDLER

#include <string>

#include "base/macros.h"
#include "base/optional.h"
#include "extensions/renderer/bindings/api_binding_types.h"
#include "v8/include/v8.h"

namespace extensions {

class ExceptionHandler {
 public:
  ExceptionHandler(const binding::AddConsoleError& add_console_error,
                   const binding::RunJSFunction& run_js);
  ~ExceptionHandler();

  void HandleException(v8::Local<v8::Context> context,
                       const std::string& message,
                       v8::TryCatch* try_catch);

  void HandleException(v8::Local<v8::Context> context,
                       const std::string& full_message,
                       v8::Local<v8::Value> exception_value);

  void SetHandlerForContext(v8::Local<v8::Context> context,
                            v8::Local<v8::Function> handler);

 private:
  v8::Local<v8::Function> GetCustomHandler(v8::Local<v8::Context> context);

  binding::AddConsoleError add_console_error_;

  binding::RunJSFunction run_js_;

  DISALLOW_COPY_AND_ASSIGN(ExceptionHandler);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_BINDINGS_EXCEPTION_HANDLER

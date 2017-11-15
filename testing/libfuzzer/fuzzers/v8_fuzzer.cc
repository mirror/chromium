// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "v8/include/libplatform/libplatform.h"
#include "v8/include/v8.h"

using v8::MaybeLocal;

struct Environment {
  Environment() {
    v8::Platform* platform = v8::platform::CreateDefaultPlatform();
    v8::V8::InitializePlatform(platform);
    v8::V8::Initialize();
    v8::Isolate::CreateParams create_params;

    create_params.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    isolate = v8::Isolate::New(create_params);
  }
  v8::Isolate* isolate;
};

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
  v8::V8::InitializeICUDefaultLocation((*argv)[0]);
  v8::V8::InitializeExternalStartupData((*argv)[0]);
  v8::V8::SetFlagsFromCommandLine(argc, *argv, true);
  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 1)
    return 0;

  static Environment* env = new Environment();
  v8::Isolate::Scope isolate_scope(env->isolate);
  v8::HandleScope handle_scope(env->isolate);
  v8::Local<v8::Context> context = v8::Context::New(env->isolate);
  v8::Context::Scope context_scope(context);

  std::string source_string =
      std::string(reinterpret_cast<const char*>(data), size);

  MaybeLocal<v8::String> source_v8_string = v8::String::NewFromUtf8(
      env->isolate, source_string.c_str(), v8::NewStringType::kNormal);

  if (source_v8_string.IsEmpty())
    return 0;

  v8::TryCatch try_catch(env->isolate);
  MaybeLocal<v8::Script> script =
      v8::Script::Compile(context, source_v8_string.ToLocalChecked());

  if (script.IsEmpty())
    return 0;

  MaybeLocal<v8::Value> result = script.ToLocalChecked()->Run(context);
  ALLOW_UNUSED_LOCAL(result);
  return 0;
}

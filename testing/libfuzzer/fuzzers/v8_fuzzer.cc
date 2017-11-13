// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "v8/include/libplatform/libplatform.h"
#include "v8/include/v8.h"

struct Environment {
  Environment() {
    platform = v8::platform::CreateDefaultPlatform();
    v8::V8::InitializePlatform(platform);
    v8::V8::Initialize();
    create_params.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    isolate = v8::Isolate::New(create_params);
  }
  v8::Platform* platform;
  v8::Isolate::CreateParams create_params;
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

  std::string src_str = std::string(reinterpret_cast<const char*>(data), size);

  auto maybe_local_src_v8_str = v8::String::NewFromUtf8(
      env->isolate, src_str.c_str(), v8::NewStringType::kNormal);

  if (maybe_local_src_v8_str.IsEmpty())
    return 0;

  v8::Local<v8::String> src_v8_str = maybe_local_src_v8_str.ToLocalChecked();
  {
    v8::TryCatch try_catch(env->isolate);

    auto maybe_local_script = v8::Script::Compile(context, src_v8_str);
    if (maybe_local_script.IsEmpty())
      return 0;
    v8::Local<v8::Script> script = maybe_local_script.ToLocalChecked();

    auto maybe_local_result = script->Run(context);
    if (maybe_local_result.IsEmpty())
      return 0;

    return 0;
  }
}

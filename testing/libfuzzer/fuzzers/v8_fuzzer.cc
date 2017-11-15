// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <time.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>

#include "base/logging.h"
#include "v8/include/libplatform/libplatform.h"
#include "v8/include/v8.h"

using v8::MaybeLocal;
using std::ref;

static const int kMaxExecutionSeconds = 10;

void terminate_execution(v8::Isolate* isolate,
                         std::mutex& mtx,
                         bool& is_running,
                         time_t& start_time) {
  while (true) {
    mtx.lock();
    if (is_running) {
      if (difftime(time(0), start_time) > kMaxExecutionSeconds) {
        isolate->TerminateExecution();
        is_running = false;
      }
    }
    mtx.unlock();
  }
}

struct Environment {
  Environment() {
    v8::Platform* platform = v8::platform::CreateDefaultPlatform();
    v8::V8::InitializePlatform(platform);
    v8::V8::Initialize();
    v8::Isolate::CreateParams create_params;

    create_params.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    isolate = v8::Isolate::New(create_params);
    terminator_thread = std::thread(terminate_execution, isolate, ref(mtx),
                                    ref(is_running), ref(start_time));
  }
  std::mutex mtx;
  std::thread terminator_thread;
  v8::Isolate* isolate;
  time_t start_time;
  bool is_running;
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

  auto local_script = script.ToLocalChecked();
  env->mtx.lock();
  env->start_time = time(0);
  env->is_running = true;
  env->mtx.unlock();

  ALLOW_UNUSED_LOCAL(local_script->Run(context));

  env->mtx.lock();
  env->is_running = false;
  env->start_time = 0;
  env->mtx.unlock();

  return 0;
}

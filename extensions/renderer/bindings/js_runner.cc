// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/bindings/js_runner.h"

#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"

namespace extensions {
namespace {

base::LazyInstance<base::ThreadLocalPointer<JSRunner>>::Leaky
    g_thread_instance = LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
JSRunner* JSRunner::Get() {
  JSRunner* js_runner = g_thread_instance.Get().Get();
  return js_runner;
}

void JSRunner::SetThreadInstance(JSRunner* js_runner) {
  g_thread_instance.Get().Set(js_runner);
}

}  // namespace extensions

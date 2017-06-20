// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/bindings/InstallRuntimeEnabledFeatures.h"

namespace blink {

namespace {

void DefaultInstallFunction(v8::Isolate*,
                            const DOMWrapperWorld&,
                            const WrapperTypeInfo*,
                            v8::Local<v8::Object>,
                            v8::Local<v8::Object>,
                            v8::Local<v8::Function>) { /* Do nothing */
}

}  // namespace

InstallRuntimeEnabledFeatures::InstallFunction
    InstallRuntimeEnabledFeatures::install_function_ = DefaultInstallFunction;

void InstallRuntimeEnabledFeatures::SetInstallFunction(InstallFunction func) {
  DCHECK(!install_function_);
  install_function_ = func;
}

void InstallRuntimeEnabledFeatures::InstallFeatures(
    v8::Isolate* isolate,
    const DOMWrapperWorld& world,
    const WrapperTypeInfo* wrapper_type_info,
    v8::Local<v8::Object> instance,
    v8::Local<v8::Object> prototype,
    v8::Local<v8::Function> interface) {
  (*install_function_)(isolate, world, wrapper_type_info, instance, prototype,
                       interface);
}

}  // namespace blink

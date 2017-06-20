// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/bindings/RuntimeEnabledPropertiesInstaller.h"

namespace blink {

namespace {

void DefaultInstallOnTemplateFunction(v8::Isolate*,
                                      const DOMWrapperWorld&,
                                      const WrapperTypeInfo*,
                                      v8::Local<v8::ObjectTemplate>,
                                      v8::Local<v8::ObjectTemplate>,
                                      v8::Local<v8::FunctionTemplate>) {
  NOTREACHED();
}

void DefaultInstallOnObjectFunction(v8::Isolate*,
                                    const DOMWrapperWorld&,
                                    const WrapperTypeInfo*,
                                    v8::Local<v8::Object>,
                                    v8::Local<v8::Object>,
                                    v8::Local<v8::Function>) {
  NOTREACHED();
}

}  // namespace

RuntimeEnabledPropertiesInstaller::InstallOnTemplateFunction
    RuntimeEnabledPropertiesInstaller::install_on_template_function_ =
        DefaultInstallOnTemplateFunction;

RuntimeEnabledPropertiesInstaller::InstallOnObjectFunction
    RuntimeEnabledPropertiesInstaller::install_on_object_function_ =
        DefaultInstallOnObjectFunction;

void RuntimeEnabledPropertiesInstaller::SetInstallFunction(
    InstallOnTemplateFunction func) {
  install_on_template_function_ = func;
}

void RuntimeEnabledPropertiesInstaller::SetInstallFunction(
    InstallOnObjectFunction func) {
  install_on_object_function_ = func;
}

void RuntimeEnabledPropertiesInstaller::InstallProperties(
    v8::Isolate* isolate,
    const DOMWrapperWorld& world,
    const WrapperTypeInfo* wrapper_type_info,
    v8::Local<v8::ObjectTemplate> instance,
    v8::Local<v8::ObjectTemplate> prototype,
    v8::Local<v8::FunctionTemplate> interface) {
  (*install_on_template_function_)(isolate, world, wrapper_type_info, instance,
                                   prototype, interface);
}

void RuntimeEnabledPropertiesInstaller::InstallProperties(
    v8::Isolate* isolate,
    const DOMWrapperWorld& world,
    const WrapperTypeInfo* wrapper_type_info,
    v8::Local<v8::Object> instance,
    v8::Local<v8::Object> prototype,
    v8::Local<v8::Function> interface) {
  (*install_on_object_function_)(isolate, world, wrapper_type_info, instance,
                                 prototype, interface);
}

}  // namespace blink

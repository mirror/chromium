// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RuntimeEnabledPropertiesInstaller_h
#define RuntimeEnabledPropertiesInstaller_h

#include "platform/PlatformExport.h"
#include "platform/wtf/Allocator.h"
#include "v8/include/v8.h"

namespace blink {

class DOMWrapperWorld;
struct WrapperTypeInfo;

// This class holds pointers to functions that install all-kinds of runtime
// enabled properties for given WrapperTypeInfo.
// Other conditional features, e.g. origin trials, are not handled in this
// class.
class PLATFORM_EXPORT RuntimeEnabledPropertiesInstaller {
  STATIC_ONLY(RuntimeEnabledPropertiesInstaller);

 public:
  using InstallOnTemplateFunction = void (*)(v8::Isolate*,
                                             const DOMWrapperWorld&,
                                             const WrapperTypeInfo*,
                                             v8::Local<v8::ObjectTemplate>,
                                             v8::Local<v8::ObjectTemplate>,
                                             v8::Local<v8::FunctionTemplate>);
  using InstallOnObjectFunction = void (*)(v8::Isolate*,
                                           const DOMWrapperWorld&,
                                           const WrapperTypeInfo*,
                                           v8::Local<v8::Object>,
                                           v8::Local<v8::Object>,
                                           v8::Local<v8::Function>);

  static void SetInstallFunction(InstallOnTemplateFunction);
  static void SetInstallFunction(InstallOnObjectFunction);

  static void InstallProperties(v8::Isolate*,
                                const DOMWrapperWorld&,
                                const WrapperTypeInfo*,
                                v8::Local<v8::Object> instance,
                                v8::Local<v8::Object> prototype,
                                v8::Local<v8::Function> interface);
  static void InstallProperties(v8::Isolate*,
                                const DOMWrapperWorld&,
                                const WrapperTypeInfo*,
                                v8::Local<v8::ObjectTemplate> instance,
                                v8::Local<v8::ObjectTemplate> prototype,
                                v8::Local<v8::FunctionTemplate> interface);

 private:
  static InstallOnTemplateFunction install_on_template_function_;
  static InstallOnObjectFunction install_on_object_function_;
};

}  // namespace blink

#endif  // RuntimeEnabledPropertiesInstaller_h

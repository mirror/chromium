// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef InstallRuntimeEnabledFeatures_h
#define InstallRuntimeEnabledFeatures_h

#include "platform/PlatformExport.h"
#include "platform/wtf/Allocator.h"
#include "v8/include/v8.h"

namespace blink {

class DOMWrapperWorld;
struct WrapperTypeInfo;

// This class holds pointers to a function that installs runtime enabled
// features on instantiated objects.
class PLATFORM_EXPORT InstallRuntimeEnabledFeatures {
  STATIC_ONLY(InstallRuntimeEnabledFeatures);

 public:
  using InstallFunction = void (*)(v8::Isolate*,
                                   const DOMWrapperWorld&,
                                   const WrapperTypeInfo*,
                                   v8::Local<v8::Object>,
                                   v8::Local<v8::Object>,
                                   v8::Local<v8::Function>);

  static void SetInstallFunction(InstallFunction);

  static void InstallFeatures(v8::Isolate*,
                              const DOMWrapperWorld&,
                              const WrapperTypeInfo*,
                              v8::Local<v8::Object> instance,
                              v8::Local<v8::Object> prototype,
                              v8::Local<v8::Function> interface);

 private:
  static InstallFunction install_function_;
};

}  // namespace blink

#endif  // InstallRuntimeEnabledFeatures_h

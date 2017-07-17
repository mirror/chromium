// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ConditionalFeatures_h
#define ConditionalFeatures_h

#include "platform/PlatformExport.h"
#include "platform/wtf/text/WTFString.h"
#include "v8/include/v8.h"

namespace blink {

class ScriptState;
struct WrapperTypeInfo;

using InstallConditionalFeaturesFunction = void (*)(const WrapperTypeInfo*,
                                                    const ScriptState*,
                                                    v8::Local<v8::Object>,
                                                    v8::Local<v8::Function>);

using InstallConditionalFeaturesOnGlobalFunction =
    void (*)(const WrapperTypeInfo*, const ScriptState*);

using InstallPendingConditionalFeatureFunction = void (*)(const String&,
                                                          const ScriptState*);

// Sets the function to be called by |installConditionalFeatures|. The function
// is initially set to the private |installConditionalFeaturesDefault| function,
// but can be overridden by this function. A pointer to the previously set
// function is returned, so that functions can be chained.
PLATFORM_EXPORT InstallConditionalFeaturesFunction
    SetInstallConditionalFeaturesFunction(InstallConditionalFeaturesFunction);

PLATFORM_EXPORT InstallConditionalFeaturesOnGlobalFunction
    SetInstallConditionalFeaturesOnGlobalFunction(
        InstallConditionalFeaturesOnGlobalFunction);

// Sets the function to be called by |installPendingConditionalFeature|. This
// is initially set to the private |installPendingConditionalFeatureDefault|
// function, but can be overridden by this function. A pointer to the previously
// set function is returned, so that functions can be chained.
PLATFORM_EXPORT InstallPendingConditionalFeatureFunction
    SetInstallPendingConditionalFeatureFunction(
        InstallPendingConditionalFeatureFunction);

// Installs all of the conditionally enabled V8 bindings for the given type, in
// a specific context. This is called in V8PerContextData, after the constructor
// and prototype for the type have been created. It indirectly calls the
// function set by |setInstallConditionalFeaturesFunction|.
PLATFORM_EXPORT void InstallConditionalFeatures(const WrapperTypeInfo*,
                                                const ScriptState*,
                                                v8::Local<v8::Object>,
                                                v8::Local<v8::Function>);

// Installs all of the conditionally enabled V8 bindings for the given global
// type (i.e. marked in IDL by [Global] or [PrimaryGlobal]). Specifically,
// global objects must have members on the instance itself, instead of the
// prototype (see https://heycam.github.io/webidl/#Global). This means the
// InstallConditionFeatures() method is insufficient for all conditional V8
// bindings.
PLATFORM_EXPORT void InstallConditionalFeaturesOnGlobal(const WrapperTypeInfo*,
                                                        const ScriptState*);

// Installs all of the conditionally enabled V8 bindings on the Window object.
// For some conditionally-enabled features (i.e. origin trials), they cannot be
// enabled until the execution context is available (e.g. parsing the document,
// inspecting HTTP headers). This can happen after the Window object has been
// setup. Thus, this method is called separately, after conditional features are
// known to be enabled. This method calls InstallConditionalFeaturesOnGlobal(),
// so a separate call is not needed.
PLATFORM_EXPORT void InstallConditionalFeaturesOnWindow(const WrapperTypeInfo*,
                                                        const ScriptState*);

// Installs all of the conditionally enabled V8 bindings for a feature, if
// needed. This is called to install a newly-enabled feature on any existing
// objects. If the target object hasn't been created, nothing is installed. The
// enabled feature will be instead be installed when the object is created
// (avoids forcing the creation of objects prematurely).
PLATFORM_EXPORT void InstallPendingConditionalFeature(const String&,
                                                      const ScriptState*);

}  // namespace blink

#endif  // ConditionalFeatures_h

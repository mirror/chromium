// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DynamicModuleResolver_h
#define DynamicModuleResolver_h

#include "core/CoreExport.h"
#include "platform/heap/Handle.h"
#include "platform/heap/Visitor.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class Modulator;
class ScriptPromiseResolver;

// DynamicModuleResolver implements "Runtime Semantics:
// HostResolveImportedModule" per HTML spec.
// https://html.spec.whatwg.org/#hostresolveimportedmodule(referencingmodule,-specifier)
class CORE_EXPORT DynamicModuleResolver
    : public GarbageCollected<DynamicModuleResolver> {
 public:
  DECLARE_TRACE();

  static DynamicModuleResolver* Create(Modulator* modulator) {
    return new DynamicModuleResolver(modulator);
  }

  void ResolveDynamically(const String& specifier,
                          const String& referrer,
                          ScriptPromiseResolver*);

 private:
  class TreeClient;

  DynamicModuleResolver(Modulator* modulator) : modulator_(modulator) {}

  Member<Modulator> modulator_;
};

}  // namespace blink

#endif  // DynamicModuleResolver_h

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WorkletModulatorImpl_h
#define WorkletModulatorImpl_h

#include "core/dom/ModulatorImplBase.h"

#include "platform/heap/Handle.h"

namespace blink {

class ModuleScriptFetcher;
class ScriptState;

class WorkletModulatorImpl final : public ModulatorImplBase {
 public:
  static ModulatorImplBase* Create(RefPtr<ScriptState>);

  // Implements Modulator.
  ModuleScriptFetcher* CreateModuleScriptFetcher() override;

 private:
  explicit WorkletModulatorImpl(RefPtr<ScriptState>);
};

}  // namespace blink

#endif  // WorkletModulatorImpl_h

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ModuleScriptFetcher_h
#define ModuleScriptFetcher_h

#include "core/CoreExport.h"
#include "core/loader/modulescript/ModuleScriptCreationParams.h"
#include "platform/loader/fetch/FetchParameters.h"
#include "platform/wtf/Optional.h"

namespace blink {

class ConsoleMessage;

// ModuleScriptFetcher emits FetchParameters to ResourceFetcher
// (via ScriptResource::Fetch). Then, it keeps track of the fetch progress by
// being a ResourceOwner. Finally, it returns its client a fetched resource as
// ModuleScriptCreationParams.
class CORE_EXPORT ModuleScriptFetcher : public GarbageCollectedMixin {
 public:
  class CORE_EXPORT Client : public GarbageCollectedMixin {
   public:
    virtual void NotifyFetchFinished(
        const WTF::Optional<ModuleScriptCreationParams>&,
        ConsoleMessage* error_message) = 0;
  };

  virtual void Fetch(FetchParameters&) = 0;
};

}  // namespace blink

#endif  // ModuleScriptFetcher_h

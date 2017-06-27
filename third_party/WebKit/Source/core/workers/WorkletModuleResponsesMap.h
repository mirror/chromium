// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WorkletModuleResponsesMap_h
#define WorkletModuleResponsesMap_h

#include "core/CoreExport.h"
#include "core/loader/modulescript/ModuleScriptData.h"
#include "platform/heap/Heap.h"
#include "platform/heap/HeapAllocator.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/KURLHash.h"
#include "platform/wtf/Functional.h"
#include "platform/wtf/Optional.h"

namespace blink {

// Implementation of the module responses map concept and the "fetch a worklet
// script" algorithm:
// https://drafts.css-houdini.org/worklets/#module-responses-map
// https://drafts.css-houdini.org/worklets/#fetch-a-worklet-script
class CORE_EXPORT WorkletModuleResponsesMap
    : public GarbageCollectedFinalized<WorkletModuleResponsesMap> {
 public:
  enum class ReadStatus {
    kOK,
    kFailed,
    kNeedsFetching,
  };
  using ReadEntryCallback =
      Function<void(ReadStatus, WTF::Optional<ModuleScriptData>),
               WTF::kSameThreadAffinity>;

  WorkletModuleResponsesMap() = default;

  // Reads an entry for a given URL or creates a new entry:
  // 1) If an entry doesn't exist, creates an empty entry and invokes the
  //    callback with an ReadStatus::kNeedsFetching. A caller is required to
  //    fetch a module script and to update the empty entry via UpdateEntry().
  // 2) If an empty entry exists, a given callback is enqueued into an entry's
  //    pending callback queue and invoked once the entry is updated.
  // 3) If a non-empty entry exists, a given callback is invoked with module
  //    script data.
  void ReadOrCreateEntry(const KURL&, std::unique_ptr<ReadEntryCallback>);

  // Updates an empty entry for a given URL with module script data.
  void UpdateEntry(const KURL&, const ModuleScriptData&);

  // Marks an entry for a given URL as "failed" and aborts queued callbacks with
  // ReadStatus::kFailed. Called when module loading for the URL fails.
  void InvalidateEntry(const KURL&);

  DECLARE_TRACE();

 private:
  class Entry;

  // TODO(nhiroki): Keep the insertion order of top-level modules to replay
  // addModule() calls for a newly created global scope.
  // See https://drafts.css-houdini.org/worklets/#creating-a-workletglobalscope
  HeapHashMap<KURL, Member<Entry>> entries_;
};

}  // namespace blink

#endif  // WorkletModuleResponsesMap_h

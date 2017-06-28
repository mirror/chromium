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

  // Reads an entry or creates a new entry for a given URL:
  // 1) If an entry doesn't exist, creates an entry in 'fetching' state and
  //    invokes the callback with ReadStatus::kNeedsFetching. A caller is
  //    required to fetch a module script and to update the entry via
  //    UpdateEntry().
  // 2) If an entry in 'fetching' state exists, a given callback is enqueued
  //    into the entry's pending callback queue and invoked once the entry is
  //    updated.
  // 3) If an entry in 'fetched' state exists, a given callback is immediately
  //    invoked with module script data.
  void ReadOrCreateEntry(const KURL&, std::unique_ptr<ReadEntryCallback>);

  // Updates an entry in 'fetching' state to 'fetched'.
  void UpdateEntry(const KURL&, const ModuleScriptData&);

  // Marks an entry as "failed" state and aborts queued callbacks with
  // ReadStatus::kFailed. Called when module loading fails.
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

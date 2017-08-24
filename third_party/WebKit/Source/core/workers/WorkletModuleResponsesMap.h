// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WorkletModuleResponsesMap_h
#define WorkletModuleResponsesMap_h

#include "core/CoreExport.h"
#include "core/loader/modulescript/ModuleScriptCreationParams.h"
#include "platform/heap/Heap.h"
#include "platform/heap/HeapAllocator.h"
#include "platform/loader/fetch/FetchParameters.h"
#include "platform/loader/fetch/ResourceFetcher.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/KURLHash.h"

namespace blink {

// WorkletModuleResponsesMap implements the module responses map concept and the
// "fetch a worklet script" algorithm:
// https://drafts.css-houdini.org/worklets/#module-responses-map
// https://drafts.css-houdini.org/worklets/#fetch-a-worklet-script
class CORE_EXPORT WorkletModuleResponsesMap
    : public GarbageCollectedFinalized<WorkletModuleResponsesMap> {
 public:
  // Used for notifying results of ReadOrCreateEntry(). See comments on the
  // function for details.
  class CORE_EXPORT Client : public GarbageCollectedMixin {
   public:
    virtual ~Client() {}
    virtual void OnRead(const ModuleScriptCreationParams&) = 0;
    virtual void OnFailed() = 0;
  };

  explicit WorkletModuleResponsesMap(ResourceFetcher*);

  // Reads an entry for a given URL. If an entry is already fetched,
  // synchronously calls Client::OnRead(). Otherwise, Client::OnRead() is called
  // on the completion of the fetch.
  void ReadEntry(const FetchParameters&, Client*);

  // Marks an entry as "failed" state and calls OnFailed() for waiting clients.
  void InvalidateEntry(const KURL&);

  // Called when the associated document is destroyed. Aborts all waiting
  // clients and clears the map. Following read and write requests to the map
  // are simply ignored.
  void Dispose();

  DECLARE_TRACE();

 private:
  class Entry;

  bool is_available_ = true;

  Member<ResourceFetcher> fetcher_;

  // TODO(nhiroki): Keep the insertion order of top-level modules to replay
  // addModule() calls for a newly created global scope.
  // See https://drafts.css-houdini.org/worklets/#creating-a-workletglobalscope
  HeapHashMap<KURL, Member<Entry>> entries_;
};

}  // namespace blink

#endif  // WorkletModuleResponsesMap_h

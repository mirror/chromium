// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/workers/WorkletModuleResponsesMap.h"

namespace blink {

class WorkletModuleResponsesMap::Entry
    : public GarbageCollectedFinalized<Entry> {
 public:
  enum class State {
    kFetching,
    kFetched,
    kFailed,
  };

  void ReadData(std::unique_ptr<ReadEntryCallback> callback) {
    switch (state_) {
      case Entry::State::kFetching:
        // Step 3: "If cache contains an entry with key url whose value is
        // "fetching", wait until that entry's value changes, then proceed to
        // the next step."
        callbacks_.push_back(std::move(callback));
        return;
      case Entry::State::kFetched:
        // Step 4: "If cache contains an entry with key url, asynchronously
        // complete this algorithm with that entry's value, and abort these
        // steps."
        (*callback)(ReadStatus::kOK, data_);
        return;
      case Entry::State::kFailed:
        // Module fetching failed before. Abort the callback.
        (*callback)(ReadStatus::kFailed, WTF::Optional<ModuleScriptData>());
        return;
    }
  }

  void NotifyFetched(const ModuleScriptData& data) {
    DCHECK_EQ(Entry::State::kFetching, state_);
    state_ = Entry::State::kFetched;
    data_ = data;
    RunCallbacks(ReadStatus::kOK, *data_);
  }

  void NotifyFailed() {
    DCHECK_EQ(Entry::State::kFetching, state_);
    state_ = Entry::State::kFailed;
    RunCallbacks(ReadStatus::kFailed, WTF::Optional<ModuleScriptData>());
  }

  DECLARE_TRACE();

 private:
  void RunCallbacks(ReadStatus status, WTF::Optional<ModuleScriptData> data) {
    Vector<std::unique_ptr<ReadEntryCallback>> callbacks;
    callbacks.swap(callbacks_);
    for (auto& callback : callbacks)
      (*callback)(status, data);
  }

  State state_ = State::kFetching;
  WTF::Optional<ModuleScriptData> data_;
  Vector<std::unique_ptr<ReadEntryCallback>> callbacks_;
};

// Implementation of the first half of the custom fetch defined in the
// "fetch a worklet script" algorithm:
// https://drafts.css-houdini.org/worklets/#fetch-a-worklet-script
//
// "To perform the fetch given request, perform the following steps:"
// Step 1: "Let cache be the moduleResponsesMap."
// Step 2: "Let url be request's url."
void WorkletModuleResponsesMap::ReadOrCreateEntry(
    const KURL& url,
    std::unique_ptr<ReadEntryCallback> callback) {
  DCHECK(IsMainThread());

  auto it = entries_.find(url);
  if (it != entries_.end()) {
    // Step 3 to 5 are implemented in ReadData().
    it->value->ReadData(std::move(callback));
    return;
  }

  // Step 5: "Create an entry in cache with key url and value "fetching"."
  entries_.insert(url, new Entry);

  // Step 6: "Fetch request."
  // Running the callback with an empty data will make the fetcher to fallback
  // to regular module loading and Write() will be called once the fetch is
  // complete.
  (*callback)(ReadStatus::kNeedsFetching, WTF::Optional<ModuleScriptData>());
}

// Implementation of the second half of the custom fetch defined in the
// "fetch a worklet script" algorithm:
// https://drafts.css-houdini.org/worklets/#fetch-a-worklet-script
void WorkletModuleResponsesMap::UpdateEntry(const KURL& url,
                                            const ModuleScriptData& data) {
  DCHECK(IsMainThread());
  DCHECK(entries_.Contains(url));
  auto it = entries_.find(url);

  // Step 7: "Let response be the result of fetch when it asynchronously
  // completes."
  // Step 8: "Set the value of the entry in cache whose key is url to response,
  // and asynchronously complete this algorithm with response."
  it->value->NotifyFetched(data);
}

void WorkletModuleResponsesMap::InvalidateEntry(const KURL& url) {
  DCHECK(IsMainThread());
  DCHECK(entries_.Contains(url));
  auto it = entries_.find(url);
  it->value->NotifyFailed();
}

DEFINE_TRACE(WorkletModuleResponsesMap) {
  visitor->Trace(entries_);
}

DEFINE_TRACE(WorkletModuleResponsesMap::Entry) {}

}  // namespace blink

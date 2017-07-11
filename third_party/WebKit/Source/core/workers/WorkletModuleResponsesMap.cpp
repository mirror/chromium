// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/workers/WorkletModuleResponsesMap.h"

#include "platform/wtf/Optional.h"

namespace blink {

struct WorkletModuleResponsesMap::Entry
    : public GarbageCollectedFinalized<Entry> {
  DEFINE_INLINE_TRACE() { visitor->Trace(clients); }

  enum class State { kFetching, kFetched, kFailed };
  State state;
  WTF::Optional<ModuleScriptCreationParams> params;
  HeapVector<Member<Client>> clients;
};

// Implementation of the first half of the custom fetch defined in the
// "fetch a worklet script" algorithm:
// https://drafts.css-houdini.org/worklets/#fetch-a-worklet-script
//
// "To perform the fetch given request, perform the following steps:"
// Step 1: "Let cache be the moduleResponsesMap."
// Step 2: "Let url be request's url."
void WorkletModuleResponsesMap::ReadOrCreateEntry(const KURL& url,
                                                  Client* client) {
  DCHECK(IsMainThread());

  auto it = entries_.find(url);
  if (it != entries_.end()) {
    Entry* entry = it->value;
    switch (entry->state) {
      case Entry::State::kFetching:
        // Step 3: "If cache contains an entry with key url whose value is
        // "fetching", wait until that entry's value changes, then proceed to
        // the next step."
        entry->clients.push_back(client);
        return;
      case Entry::State::kFetched:
        // Step 4: "If cache contains an entry with key url, asynchronously
        // complete this algorithm with that entry's value, and abort these
        // steps."
        client->OnRead(*entry->params);
        return;
      case Entry::State::kFailed:
        // Module fetching failed before. Abort following steps.
        client->OnFailed();
        return;
    }
  }

  // Step 5: "Create an entry in cache with key url and value "fetching"."
  entries_.insert(url, new Entry);

  // Step 6: "Fetch request."
  // Running the callback with an empty params will make the fetcher to fallback
  // to regular module loading and Write() will be called once the fetch is
  // complete.
  client->OnFetchNeeded();
}

// Implementation of the second half of the custom fetch defined in the
// "fetch a worklet script" algorithm:
// https://drafts.css-houdini.org/worklets/#fetch-a-worklet-script
void WorkletModuleResponsesMap::UpdateEntry(
    const KURL& url,
    const ModuleScriptCreationParams& params) {
  DCHECK(IsMainThread());
  DCHECK(entries_.Contains(url));
  Entry* entry = entries_.find(url)->value;

  DCHECK_EQ(Entry::State::kFetching, entry->state);
  entry->state = Entry::State::kFetched;

  // Step 7: "Let response be the result of fetch when it asynchronously
  // completes."
  // Step 8: "Set the value of the entry in cache whose key is url to response,
  // and asynchronously complete this algorithm with response."
  entry->params.emplace(params);
  HeapVector<Member<Client>> clients;
  clients.swap(entry->clients);
  for (Client* client : clients)
    client->OnRead(*entry->params);
}

void WorkletModuleResponsesMap::InvalidateEntry(const KURL& url) {
  DCHECK(IsMainThread());
  DCHECK(entries_.Contains(url));
  Entry* entry = entries_.find(url)->value;

  DCHECK_EQ(Entry::State::kFetching, entry->state);
  entry->state = Entry::State::kFailed;

  HeapVector<Member<Client>> clients;
  clients.swap(entry->clients);
  for (Client* client : clients)
    client->OnFailed();
}

DEFINE_TRACE(WorkletModuleResponsesMap) {
  visitor->Trace(entries_);
}

}  // namespace blink

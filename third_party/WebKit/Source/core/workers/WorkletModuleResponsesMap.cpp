// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/workers/WorkletModuleResponsesMap.h"

#include "core/inspector/ConsoleMessage.h"
#include "core/loader/modulescript/ModuleScriptFetcher.h"
#include "core/loader/resource/ScriptResource.h"
#include "platform/loader/fetch/ResourceOwner.h"
#include "platform/wtf/Optional.h"

namespace blink {

namespace {

bool IsValidURL(const KURL& url) {
  return !url.IsEmpty() && url.IsValid();
}

}  // namespace

class WorkletModuleResponsesMap::Entry
    : public GarbageCollectedFinalized<Entry>,
      public ResourceOwner<ScriptResource> {
  USING_GARBAGE_COLLECTED_MIXIN(WorkletModuleResponsesMap::Entry);

 public:
  enum class State { kFetching, kFetched, kFailed };

  Entry(const FetchParameters fetch_params, ResourceFetcher* fetcher)
      : fetch_params_(fetch_params), fetcher_(fetcher) {}

  void Fetch() {
    ScriptResource* resource = ScriptResource::Fetch(fetch_params_, fetcher_);
    if (state_ == State::kFetched || state_ == State::kFailed) {
      // ScriptResource::Fetch() has succeeded syhnchronously,
      // ::NotifyFinished() already took care of the |resource|.
      return;
    }
    if (!resource) {
      // ScriptResource::Fetch() has failed synchronously.
      NotifyFinished(nullptr);
      return;
    }
    // ScriptResource::Fetch() is processed asynchronously.
    SetResource(resource);
  }

  State GetState() const { return state_; }
  ModuleScriptCreationParams GetParams() const { return *params_; }

  void AddClient(Client* client) {
    // Clients can be added only while a module script is being fetched.
    DCHECK_EQ(State::kFetching, state_);
    clients_.push_back(client);
  }

  // Implements ResourceClient.
  // Implementation of the second half of the custom fetch defined in the
  // "fetch a worklet script" algorithm:
  // https://drafts.css-houdini.org/worklets/#fetch-a-worklet-script
  void NotifyFinished(Resource* resource) override {
    ClearResource();

    ScriptResource* script_resource = ToScriptResource(resource);
    ConsoleMessage* error_message = nullptr;
    if (!ModuleScriptFetcher::VerifyFetchedModule(script_resource,
                                                  &error_message)) {
      NotifyFailure(error_message);
      return;
    }

    // The entry can be disposed of during the resource fetch.
    if (state_ == State::kFailed)
      return;

    // Step 7: "Let response be the result of fetch when it asynchronously
    // completes."
    // Step 8: "Set the value of the entry in cache whose key is url to
    // response, and asynchronously complete this algorithm with response."
    AdvanceState(State::kFetched);
    ModuleScriptCreationParams params(
        script_resource->GetResponse().Url(), script_resource->SourceText(),
        script_resource->GetResourceRequest().GetFetchCredentialsMode(),
        script_resource->CalculateAccessControlStatus());
    params_.emplace(params);
    for (Client* client : clients_)
      client->OnRead(params);
    clients_.clear();
  }
  String DebugName() const final { return "WorkletModuleResponsesMap::Entry"; }

  void NotifyFailure(ConsoleMessage* error_message) {
    AdvanceState(State::kFailed);
    for (Client* client : clients_)
      client->OnFailed(error_message);
    clients_.clear();
  }

  DEFINE_INLINE_TRACE() {
    visitor->Trace(fetcher_);
    visitor->Trace(clients_);
  }

 private:
  void AdvanceState(State new_state) {
    switch (state_) {
      case State::kFetching:
        DCHECK(new_state == State::kFetched || new_state == State::kFailed);
        break;
      case State::kFetched:
      case State::kFailed:
        NOTREACHED();
        break;
    }
    state_ = new_state;
  }

  State state_ = State::kFetching;

  FetchParameters fetch_params_;
  Member<ResourceFetcher> fetcher_;

  WTF::Optional<ModuleScriptCreationParams> params_;
  HeapVector<Member<Client>> clients_;
};

WorkletModuleResponsesMap::WorkletModuleResponsesMap(ResourceFetcher* fetcher)
    : fetcher_(fetcher) {}

// Implementation of the first half of the custom fetch defined in the
// "fetch a worklet script" algorithm:
// https://drafts.css-houdini.org/worklets/#fetch-a-worklet-script
//
// "To perform the fetch given request, perform the following steps:"
// Step 1: "Let cache be the moduleResponsesMap."
// Step 2: "Let url be request's url."
void WorkletModuleResponsesMap::ReadOrCreateEntry(
    const FetchParameters& fetch_params,
    Client* client) {
  DCHECK(IsMainThread());
  if (!is_available_ || !IsValidURL(fetch_params.Url())) {
    client->OnFailed(nullptr);
    return;
  }

  auto it = entries_.find(fetch_params.Url());
  if (it != entries_.end()) {
    Entry* entry = it->value;
    switch (entry->GetState()) {
      case Entry::State::kFetching:
        // Step 3: "If cache contains an entry with key url whose value is
        // "fetching", wait until that entry's value changes, then proceed to
        // the next step."
        entry->AddClient(client);
        return;
      case Entry::State::kFetched:
        // Step 4: "If cache contains an entry with key url, asynchronously
        // complete this algorithm with that entry's value, and abort these
        // steps."
        client->OnRead(entry->GetParams());
        return;
      case Entry::State::kFailed:
        // Module fetching failed before. Abort following steps.
        client->OnFailed(nullptr);
        return;
    }
  }

  // Step 5: "Create an entry in cache with key url and value "fetching"."
  Entry* entry = new Entry(fetch_params, fetcher_.Get());
  entry->AddClient(client);
  entries_.insert(fetch_params.Url(), entry);

  // Step 6: "Fetch request."
  // Running the callback with an empty params will make the fetcher to fallback
  // to regular module loading and Write() will be called once the fetch is
  // complete.
  entry->Fetch();
}

void WorkletModuleResponsesMap::InvalidateEntry(const KURL& url) {
  DCHECK(IsMainThread());
  DCHECK(IsValidURL(url));
  if (!is_available_)
    return;

  DCHECK(entries_.Contains(url));
  Entry* entry = entries_.find(url)->value;
  entry->NotifyFailure(nullptr);
}

void WorkletModuleResponsesMap::Dispose() {
  is_available_ = false;
  for (auto it : entries_) {
    switch (it.value->GetState()) {
      case Entry::State::kFetching:
        it.value->NotifyFailure(nullptr);
        break;
      case Entry::State::kFetched:
      case Entry::State::kFailed:
        break;
    }
  }
  entries_.clear();
}

DEFINE_TRACE(WorkletModuleResponsesMap) {
  visitor->Trace(fetcher_);
  visitor->Trace(entries_);
}

}  // namespace blink

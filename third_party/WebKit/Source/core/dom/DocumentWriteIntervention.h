// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DocumentWriteIntervention_h
#define DocumentWriteIntervention_h

#include "core/loader/resource/ScriptResource.h"
#include "platform/loader/fetch/FetchParameters.h"
#include "platform/loader/fetch/ResourceOwner.h"

namespace blink {

class Document;

// If the script was blocked as part of document.write intervention,
// then send an asynchronous GET request with an interventions header.
// This is done by saving the original FetchParameters in
// FetchBlockedDocWriteScriptClient when sending intervened request is sent
// and resending a request using the saved FetchParameters + some modifications
// in NotifyFinished() when the request turns actually blocked.
// This is done separately from the code path for script execution, and
// ClassicPendingScript will be notified separately.
class FetchBlockedDocWriteScriptClient
    : public GarbageCollectedFinalized<FetchBlockedDocWriteScriptClient>,
      public ResourceOwner<ScriptResource> {
  USING_GARBAGE_COLLECTED_MIXIN(FetchBlockedDocWriteScriptClient);

 public:
  FetchBlockedDocWriteScriptClient(Document& document,
                                   const FetchParameters& params)
      : document_(&document), params_(params) {}

  using ResourceOwner<ScriptResource>::SetResource;

  DECLARE_VIRTUAL_TRACE();

 private:
  void NotifyFinished(Resource*) override;
  String DebugName() const override {
    return "FetchBlockedDocWriteScriptClient";
  }

  WeakMember<Document> document_;
  FetchParameters params_;
};

// Returns non-null if the fetch should be blocked due to the document.write
// intervention. In that case, the request's cache policy is set to
// kReturnCacheDataDontLoad to ensure a network request is not generated.
// This function may also set an Intervention header, log the intervention in
// the console, etc.
//
// This only affects scripts added via document.write() in the main frame.
// The caller should call SetResource() for the returned client, and then
// asynchronous GET request to the blocked script will be sent on its
// NotifyFinished().
FetchBlockedDocWriteScriptClient* MaybeDisallowFetchForDocWrittenScript(
    FetchParameters&,
    Document&);

}  // namespace blink

#endif  // DocumentWriteIntervention_h

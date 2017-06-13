// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/ClassicPendingScript.h"

#include "bindings/core/v8/ScriptSourceCode.h"
#include "bindings/core/v8/ScriptStreamer.h"
#include "bindings/core/v8/V8BindingForCore.h"
#include "core/dom/Document.h"
#include "core/dom/ScriptRunner.h"
#include "core/dom/TaskRunnerHelper.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/SubresourceIntegrity.h"
#include "platform/bindings/ScriptState.h"
#include "platform/loader/fetch/MemoryCache.h"

namespace blink {

ClassicPendingScript* ClassicPendingScript::Create(ScriptElementBase* element,
                                                   ScriptResource* resource) {
  return new ClassicPendingScript(element, resource, TextPosition());
}

ClassicPendingScript* ClassicPendingScript::Create(
    ScriptElementBase* element,
    const TextPosition& starting_position) {
  return new ClassicPendingScript(element, nullptr, starting_position);
}

ClassicPendingScript::ClassicPendingScript(
    ScriptElementBase* element,
    ScriptResource* resource,
    const TextPosition& starting_position)
    : PendingScript(element, starting_position),
      ready_state_(resource ? kWaitingForResource : kReady),
      integrity_failure_(false) {
  CheckState();
  SetResource(resource);
  MemoryCoordinator::Instance().RegisterClient(this);
}

ClassicPendingScript::~ClassicPendingScript() {}

NOINLINE void ClassicPendingScript::CheckState() const {
  // TODO(hiroshige): Turn these CHECK()s into DCHECK() before going to beta.
  CHECK(!prefinalizer_called_);
  CHECK(GetElement());
  CHECK(GetResource() || !streamer_);
  CHECK(!streamer_ || streamer_->GetResource() == GetResource());
}

void ClassicPendingScript::Prefinalize() {
  // TODO(hiroshige): Consider moving this to ScriptStreamer's prefinalizer.
  // https://crbug.com/715309
  if (streamer_)
    streamer_->Cancel();
  prefinalizer_called_ = true;
}

void ClassicPendingScript::DisposeInternal() {
  MemoryCoordinator::Instance().UnregisterClient(this);
  SetResource(nullptr);
  integrity_failure_ = false;
  if (streamer_)
    streamer_->Cancel();
  streamer_ = nullptr;
}

void ClassicPendingScript::StreamingFinished() {
  CheckState();
  DCHECK(streamer_);  // Should only be called by ScriptStreamer.

  if (ready_state_ == kWaitingForStreaming) {
    FinishWaitingForStreaming();
  } else if (ready_state_ == kReadyStreaming) {
    FinishReadyStreaming();
  } else {
    UNREACHABLE();
  }

  // Inform the streaming requester, if desired.
  if (streamer_done_)
    (*streamer_done_)();
}

void ClassicPendingScript::FinishWaitingForStreaming() {
  CheckState();
  DCHECK(GetResource());
  DCHECK_EQ(ready_state_, kWaitingForStreaming);

  bool error_occurred = GetResource()->ErrorOccurred() || integrity_failure_;
  AdvanceReadyState(error_occurred ? kErrorOccurred : kReady);
}

void ClassicPendingScript::FinishReadyStreaming() {
  CheckState();
  DCHECK(GetResource());
  DCHECK_EQ(ready_state_, kReadyStreaming);
  AdvanceReadyState(kReady);
}

// Returns true if SRI check passed.
static bool CheckScriptResourceIntegrity(Resource* resource,
                                         ScriptElementBase* element) {
  DCHECK_EQ(resource->GetType(), Resource::kScript);
  ScriptResource* script_resource = ToScriptResource(resource);
  String integrity_attr = element->IntegrityAttributeValue();

  // It is possible to get back a script resource with integrity metadata
  // for a request with an empty integrity attribute. In that case, the
  // integrity check should be skipped, so this check ensures that the
  // integrity attribute isn't empty in addition to checking if the
  // resource has empty integrity metadata.
  if (integrity_attr.IsEmpty() ||
      script_resource->IntegrityMetadata().IsEmpty())
    return true;

  switch (script_resource->IntegrityDisposition()) {
    case ResourceIntegrityDisposition::kPassed:
      return true;

    case ResourceIntegrityDisposition::kFailed:
      // TODO(jww): This should probably also generate a console
      // message identical to the one produced by
      // CheckSubresourceIntegrity below. See https://crbug.com/585267.
      return false;

    case ResourceIntegrityDisposition::kNotChecked: {
      if (!resource->ResourceBuffer())
        return true;

      bool passed = SubresourceIntegrity::CheckSubresourceIntegrity(
          script_resource->IntegrityMetadata(), element->GetDocument(),
          resource->ResourceBuffer()->Data(),
          resource->ResourceBuffer()->size(), resource->Url(), *resource);
      script_resource->SetIntegrityDisposition(
          passed ? ResourceIntegrityDisposition::kPassed
                 : ResourceIntegrityDisposition::kFailed);
      return passed;
    }
  }

  NOTREACHED();
  return true;
}

void ClassicPendingScript::NotifyFinished(Resource* resource) {
  // The following SRI checks need to be here because, unfortunately, fetches
  // are not done purely according to the Fetch spec. In particular,
  // different requests for the same resource do not have different
  // responses; the memory cache can (and will) return the exact same
  // Resource object.
  //
  // For different requests, the same Resource object will be returned and
  // will not be associated with the particular request.  Therefore, when the
  // body of the response comes in, there's no way to validate the integrity
  // of the Resource object against a particular request (since there may be
  // several pending requests all tied to the identical object, and the
  // actual requests are not stored).
  //
  // In order to simulate the correct behavior, Blink explicitly does the SRI
  // checks here, when a PendingScript tied to a particular request is
  // finished (and in the case of a StyleSheet, at the point of execution),
  // while having proper Fetch checks in the fetch module for use in the
  // fetch JavaScript API. In a future world where the ResourceFetcher uses
  // the Fetch algorithm, this should be fixed by having separate Response
  // objects (perhaps attached to identical Resource objects) per request.
  //
  // See https://crbug.com/500701 for more information.
  CheckState();
  if (GetElement()) {
    integrity_failure_ = !CheckScriptResourceIntegrity(resource, GetElement());
  }

  // We are now waiting for script streaming to finish.
  // If there is no script streamer, this step completes immediately.
  AdvanceReadyState(kWaitingForStreaming);
  if (streamer_)
    streamer_->NotifyFinished(resource);
  else
    FinishWaitingForStreaming();
}

void ClassicPendingScript::NotifyAppendData(ScriptResource* resource) {
  if (streamer_)
    streamer_->NotifyAppendData(resource);
}

DEFINE_TRACE(ClassicPendingScript) {
  visitor->Trace(streamer_);
  ResourceOwner<ScriptResource>::Trace(visitor);
  MemoryCoordinatorClient::Trace(visitor);
  PendingScript::Trace(visitor);
}

ClassicScript* ClassicPendingScript::GetSource(const KURL& document_url,
                                               bool& error_occurred) const {
  CheckState();
  DCHECK(IsReady());

  error_occurred = ErrorOccurred();
  if (!GetResource()) {
    return ClassicScript::Create(ScriptSourceCode(
        GetElement()->TextContent(), document_url, StartingPosition()));
  }

  DCHECK(GetResource()->IsLoaded());
  bool streamer_ready = (ready_state_ == kReady) && streamer_ &&
                        !streamer_->StreamingSuppressed();
  return ClassicScript::Create(
      ScriptSourceCode(streamer_ready ? streamer_ : nullptr, GetResource()));
}

void ClassicPendingScript::SetStreamer(ScriptStreamer* streamer) {
  DCHECK(!streamer_);
  DCHECK(!IsWatchingForLoad());
  DCHECK(!streamer->IsFinished());
  DCHECK(ready_state_ == kWaitingForResource || ready_state_ == kReady);

  streamer_ = streamer;
  if (streamer && ready_state_ == kReady)
    AdvanceReadyState(kReadyStreaming);

  CheckState();
}

bool ClassicPendingScript::IsReady() const {
  CheckState();
  return ready_state_ >= kReady;
}

bool ClassicPendingScript::ErrorOccurred() const {
  CheckState();
  return ready_state_ == kErrorOccurred;
}

void ClassicPendingScript::AdvanceReadyState(ReadyState new_ready_state) {
  // We will allow exactly these state transitions:
  //
  // kWaitingForResource -> kWaitingForStreaming -> [kReady, kErrorOccurred]
  // kReady -> kReadyStreaming -> kReady
  switch (ready_state_) {
    case kWaitingForResource:
      CHECK_EQ(new_ready_state, kWaitingForStreaming);
      break;
    case kWaitingForStreaming:
      CHECK(new_ready_state == kReady || new_ready_state == kErrorOccurred);
      break;
    case kReady:
      CHECK_EQ(new_ready_state, kReadyStreaming);
      break;
    case kReadyStreaming:
      CHECK_EQ(new_ready_state, kReady);
      break;
    case kErrorOccurred:
      UNREACHABLE();
      break;
  }

  bool old_is_ready = IsReady();
  ready_state_ = new_ready_state;

  // Did we transition into a 'ready' state?
  if (IsReady() && !old_is_ready && IsWatchingForLoad())
    Client()->PendingScriptFinished(this);
}

void ClassicPendingScript::OnPurgeMemory() {
  CheckState();
  if (!streamer_)
    return;
  streamer_->Cancel();
  streamer_ = nullptr;
}

bool ClassicPendingScript::StartStreamingIfPossible(
    Document* document,
    ScriptStreamer::Type streamer_type,
    std::unique_ptr<WTF::Closure> done) {
  streamer_done_ = std::move(done);

  // Don't stream a script twice.
  if (streamer_)
    return false;

  // We can start streaming in two states: While still loading
  // (kWaitingForDocument), or after having loaded (kReady).
  if (ready_state_ != kWaitingForResource && ready_state_ != kReady)
    return false;

  if (!document->GetFrame())
    return false;

  ScriptState* script_state = ToScriptStateForMainWorld(document->GetFrame());
  if (!script_state)
    return false;

  // Parser blocking scripts tend to do a lot of work in the 'finished'
  // callbacks, while async + in-order scripts all do control-like activities
  // (like posting new tasks). Use the 'control' queue only for control tasks.
  // (More details in discussion for cl 500147.)
  auto task_type = streamer_type == ScriptStreamer::kParsingBlocking
                       ? TaskType::kNetworking
                       : TaskType::kNetworkingControl;
  ScriptStreamer::StartStreaming(
      this, streamer_type, document->GetFrame()->GetSettings(), script_state,
      TaskRunnerHelper::Get(task_type, document));

  // If successful, the streamer will have called SetStreamer on this
  // instance during the StartStreaming producedure. So that's how we know.
  return streamer_;
}

bool ClassicPendingScript::HasStreamer() const {
  return streamer_;
}

bool ClassicPendingScript::IsCurrentlyStreaming() const {
  // We could either check our local state, or ask the streamer. Let's
  // double-check these are consistent.
  DCHECK_EQ(streamer_ && !streamer_->IsFinished(),
            ready_state_ == kReadyStreaming || (streamer_ && !IsReady()));
  return ready_state_ == kReadyStreaming || (streamer_ && !IsReady());
}

bool ClassicPendingScript::WasCanceled() const {
  return GetResource()->WasCanceled();
}

KURL ClassicPendingScript::UrlForClassicScript() const {
  return GetResource()->Url();
}

void ClassicPendingScript::RemoveFromMemoryCache() {
  GetMemoryCache()->Remove(GetResource());
}

}  // namespace blink

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/modulescript/ModuleTreeLinker.h"

#include "bindings/core/v8/ScriptModule.h"
#include "core/dom/ModuleScript.h"
#include "core/loader/modulescript/ModuleScriptFetchRequest.h"
#include "core/loader/modulescript/ModuleTreeLinkerRegistry.h"
#include "platform/WebTaskRunner.h"
#include "platform/bindings/V8ThrowException.h"
#include "platform/loader/fetch/ResourceLoadingLog.h"
#include "platform/wtf/Vector.h"
#include "v8/include/v8.h"

namespace blink {

// Spec links:
// [FDaI]
// https://html.spec.whatwg.org/multipage/webappapis.html##fetch-the-descendants-of-and-instantiate-a-module-script
// [FD]
// https://html.spec.whatwg.org/multipage/webappapis.html#fetch-the-descendants-of-a-module-script
// [IMSGF]
// https://html.spec.whatwg.org/multipage/webappapis.html#internal-module-script-graph-fetching-procedure

ModuleTreeLinker* ModuleTreeLinker::Fetch(
    const ModuleScriptFetchRequest& request,
    Modulator* modulator,
    ModuleTreeLinkerRegistry* registry,
    ModuleTreeClient* client) {
  ModuleTreeLinker* fetcher = new ModuleTreeLinker(modulator, registry, client);
  fetcher->FetchRoot(request);
  return fetcher;
}

ModuleTreeLinker* ModuleTreeLinker::FetchDescendantsForInlineScript(
    ModuleScript* module_script,
    Modulator* modulator,
    ModuleTreeLinkerRegistry* registry,
    ModuleTreeClient* client) {
  DCHECK(module_script);
  ModuleTreeLinker* fetcher = new ModuleTreeLinker(modulator, registry, client);
  fetcher->FetchRootInline(module_script);
  return fetcher;
}

ModuleTreeLinker::ModuleTreeLinker(Modulator* modulator,
                                   ModuleTreeLinkerRegistry* registry,
                                   ModuleTreeClient* client)
    : modulator_(modulator),
      registry_(registry),
      client_(client),
      result_(nullptr) {
  CHECK(modulator);
  CHECK(registry);
  CHECK(client);
}

DEFINE_TRACE(ModuleTreeLinker) {
  visitor->Trace(modulator_);
  visitor->Trace(registry_);
  visitor->Trace(client_);
  visitor->Trace(result_);
  SingleModuleClient::Trace(visitor);
}

DEFINE_TRACE_WRAPPERS(ModuleTreeLinker) {
  visitor->TraceWrappers(result_);
}

#if DCHECK_IS_ON()
const char* ModuleTreeLinker::StateToString(ModuleTreeLinker::State state) {
  switch (state) {
    case State::kInitial:
      return "Initial";
    case State::kFetchingSelf:
      return "FetchingSelf";
    case State::kFetchingDependencies:
      return "FetchingDependencies";
    case State::kInstantiating:
      return "Instantiating";
    case State::kFinished:
      return "Finished";
  }
  NOTREACHED();
  return "";
}
#endif

void ModuleTreeLinker::AdvanceState(State new_state) {
#if DCHECK_IS_ON()
  RESOURCE_LOADING_DVLOG(1)
      << "ModuleTreeLinker[" << this << "]::advanceState("
      << StateToString(state_) << " -> " << StateToString(new_state) << ")";
#endif

  switch (state_) {
    case State::kInitial:
      CHECK_EQ(num_incomplete_fetches_, 0u);
      CHECK_EQ(new_state, State::kFetchingSelf);
      break;
    case State::kFetchingSelf:
      CHECK_EQ(num_incomplete_fetches_, 0u);
      CHECK(new_state == State::kFetchingDependencies ||
            new_state == State::kFinished);
      break;
    case State::kFetchingDependencies:
      CHECK(new_state == State::kInstantiating ||
            new_state == State::kFinished);
      break;
    case State::kInstantiating:
      CHECK_EQ(new_state, State::kFinished);
      break;
    case State::kFinished:
      NOTREACHED();
      break;
  }

  state_ = new_state;

  if (state_ == State::kFinished) {
    if (result_) {
      RESOURCE_LOADING_DVLOG(1) << "ModuleTreeLinker[" << this
                                << "] finished with final result " << *result_;
    } else {
      RESOURCE_LOADING_DVLOG(1)
          << "ModuleTreeLinker[" << this << "] finished with nullptr.";
    }

    registry_->ReleaseFinishedFetcher(this);

    // [IMSGF] Step 6. When the appropriate algorithm asynchronously completes
    // with final result, asynchronously complete this algorithm with final
    // result.
    client_->NotifyModuleTreeLoadFinished(result_);
  }
}

void ModuleTreeLinker::FetchRoot(const ModuleScriptFetchRequest& request) {
  // https://html.spec.whatwg.org/multipage/webappapis.html#internal-module-script-graph-fetching-procedure

  // [IMSGF] Step 1. Fetch a single module script ...
  AdvanceState(State::kFetchingSelf);
  FetchSingle(request, ModuleGraphLevel::kTopLevelModuleFetch);

  // [IMSGF] Step 2. Return from this algorithm, and run the following steps
  // when fetching a single module script asynchronously completes with result.
  // Note: Modulator::FetchSingle asynchronously notifies result to
  // ModuleTreeLinker::NotifyModuleLoadFinished().
}

void ModuleTreeLinker::FetchRootInline(ModuleScript* module_script) {
  // Substep 4 in "module" case in Step 22 of "prepare a script":"
  // https://html.spec.whatwg.org/#prepare-a-script

  AdvanceState(State::kFetchingSelf);
  ++num_incomplete_fetches_;

  // 4. "Fetch the descendants of and instantiate script, given the
  //     destination "script"."
  // "When this asynchronously completes, set the script's script to
  //  the result. At that time, the script is ready."
  modulator_->TaskRunner()->PostTask(
      BLINK_FROM_HERE,
      WTF::Bind(&ModuleTreeLinker::NotifyModuleLoadFinished,
                WrapPersistent(this), WrapPersistent(module_script)));
}

void ModuleTreeLinker::FetchSingle(const ModuleScriptFetchRequest& request,
                                   ModuleGraphLevel level) {
  DCHECK(!reached_url_set_.Contains(request.Url()));
  reached_url_set_.insert(request.Url());

  ++num_incomplete_fetches_;
  modulator_->FetchSingle(request, level, this);
}

void ModuleTreeLinker::CompleteFetchDescendantsForOneModuleScript() {
  // [FD] of a single module script is completed here.

  // [FD] Step 7. Wait for all invocations of the internal module script
  // graph fetching procedure to asynchronously complete, ...

  // If |num_incomplete_fetches_| is 0, then the whole process of
  // [FD] (called from [FDaI] Step 1) of the root module script
  // is completed here, and thus we proceed to [FDaI] Step 3.
  //
  // [Optimization] The latter part of [FD] Step 7. is deferred
  // to Instantiate().
  if (num_incomplete_fetches_ == 0)
    Instantiate();
}

// Code:
//   FetchRoot.*() -------> NotifyModuleLoadFinished() --> Instantiate()
//                                                           PropagateError()
// Spec:
//   -[prepare a script] ----->                               [FD]Step 7.1/2
//   -[top-level IMSGF] ------>
//                           ----[FDaI]-------------------------------------->
//                           ----[FD]---->
//                         +-->          --[IMSGF]---+
//                         |                         |
//                         +-------------<-----------+
//
// [FD] and [IMSGF] merged together, with some mixins from [FDaI] callers.
// When all descendant modules under the root module are loaded, then proceeds
// to Instantiate().
void ModuleTreeLinker::NotifyModuleLoadFinished(ModuleScript* module_script) {
  // [IMSGF] Step 2. Return from this algorithm, and run the following
  // steps when fetching a single module script asynchronously completes
  // with result:

  CHECK_GT(num_incomplete_fetches_, 0u);
  --num_incomplete_fetches_;

  if (module_script) {
    RESOURCE_LOADING_DVLOG(1)
        << "ModuleTreeLinker[" << this << "]::NotifyModuleLoadFinished() with "
        << *module_script;
  } else {
    RESOURCE_LOADING_DVLOG(1)
        << "ModuleTreeLinker[" << this << "]::NotifyModuleLoadFinished() with "
        << "nullptr.";
  }

  if (state_ == State::kFetchingSelf) {
    // Corresponds to top-level calls to
    // https://html.spec.whatwg.org/multipage/webappapis.html#fetch-the-descendants-of-and-instantiate-a-module-script
    // i.e. [IMSGF] with the top-level module fetch flag set (external), or
    // Step 22 of "prepare a script" (inline).
    // |module_script| is the top-level module, and will be instantiated
    // and returned later.
    result_ = module_script;
    AdvanceState(State::kFetchingDependencies);
  }

  if (state_ != State::kFetchingDependencies) {
    // We may reach here if one of the descendant failed to load, and the other
    // descendants fetches were in flight.
    return;
  }

  // Note: top-level module fetch flag is implemented so that Instantiate()
  // is called once after all descendants are fetched, which corresponds to
  // the single invocation of "fetch the descendants of and instantiate".

  // https://html.spec.whatwg.org/multipage/webappapis.html#fetch-the-descendants-of-a-module-script
  // [nospec] Abort the steps if the browsing context is discarded.
  if (!modulator_->HasValidContext()) {
    result_ = nullptr;
    AdvanceState(State::kFinished);
    return;
  }

  // [IMSGF] Steps 3 and 4 are merged below.

  // [IMSGF] Step 5. If the top-level module fetch flag is set, fetch the
  // descendants of and instantiate result given destination and an ancestor
  // list obtained by appending url to ancestor list. Otherwise, fetch the
  // descendants of result given the same arguments. [IMSGF] Step 6. When the
  // appropriate algorithm asynchronously completes with final result,
  // asynchronously complete this algorithm with final result.

  // Start of [FD].

  // [FD] Step 1. If ancestor list was not given, let it be the empty list.
  // [not spec'ed] ModuleTreeLinker uses |reached_url_set_| to avoid cycles
  // instead of ancestor list, and thus this step is ignored.

  // [FD] Step 2. If module script is errored or has instantiated,
  // asynchronously complete this algorithm with module script, and abort these
  // steps. [IMSGF] Step 3. If result is null, is errored, or has instantiated,
  // asynchronously complete this algorithm with result, and abort these steps.
  if (!module_script || module_script->IsErrored()) {
    found_error_ = true;
    // We don't early-exit here and wait until all module scripts to be
    // loaded, because we might be not sure which error to be reported.
    // TODO(hiroshige): It is possible to determine whether the error to be
    // reported can be determined without waiting for loading module scripts,
    // and thus to early-exit here if possible. However, the complexity of
    // such early-exit implementation might be high, and optimizing error
    // cases with the implementation cost might be not worth doing.
    CompleteFetchDescendantsForOneModuleScript();
    return;
  }
  if (module_script->HasInstantiated()) {
    CompleteFetchDescendantsForOneModuleScript();
    return;
  }

  // [IMSGF] Step 4. Assert: result's module record's [[Status]] is
  // "uninstantiated".
  DCHECK_EQ(ScriptModuleState::kUninstantiated, module_script->RecordStatus());

  // [FD] Step 3. Let record be module script's module record.
  ScriptModule record = module_script->Record();
  DCHECK(!record.IsNull());

  // [FD] Step 4. If record.[[RequestedModules]] is empty, asynchronously
  // complete this algorithm with module script. Note: We defer this bail-out
  // until the end of the procedure. The rest of
  //       the procedure will be no-op anyway if record.[[RequestedModules]]
  //       is empty.

  // [FD] Step 5. Let urls be a new empty list.
  Vector<KURL> urls;
  Vector<TextPosition> positions;

  // [FD] Step 6. For each string requested of record.[[RequestedModules]],
  Vector<Modulator::ModuleRequest> module_requests =
      modulator_->ModuleRequestsFromScriptModule(record);
  for (const auto& module_request : module_requests) {
    // [FD] Step 6.1. Let url be the result of resolving a module specifier
    // given module script and requested.
    KURL url = Modulator::ResolveModuleSpecifier(module_request.specifier,
                                                 module_script->BaseURL());

    // [FD] Step 6.2. Assert: url is never failure, because resolving a module
    // specifier must have been previously successful with these same two
    // arguments.
    CHECK(url.IsValid()) << "Modulator::resolveModuleSpecifier() impl must "
                            "return either a valid url or null.";

    // [FD] Step 6.3. If ancestor list does not contain url, append url to urls.
    // [unspec] If we already have started a sub-graph fetch for the |url| for
    // this top-level module graph starting from |top_level_linker_|, we can
    // safely rely on other module graph node ModuleTreeLinker to handle it.
    if (reached_url_set_.Contains(url))
      continue;

    urls.push_back(url);
    positions.push_back(module_request.position);
  }

  // [FD] Step 7. For each url in urls, perform the internal module script graph
  // fetching procedure given url, module script's credentials mode, module
  // script's cryptographic nonce, module script's parser state, destination,
  // module script's settings object, module script's settings object, ancestor
  // list, module script's base URL, and with the top-level module fetch flag
  // unset. If the caller of this algorithm specified custom perform the fetch
  // steps, pass those along while performing the internal module script graph
  // fetching procedure.

  if (urls.IsEmpty()) {
    // [FD] Step 4. If record.[[RequestedModules]] is empty, asynchronously
    // complete this algorithm with module script.
    //
    // Also, if record.[[RequestedModules]] is not empty but |urls| is
    // empty here, we complete this algorithm.
    CompleteFetchDescendantsForOneModuleScript();
    return;
  }

  // [FD] Step 7, when "urls" is non-empty.
  // These invocations of the internal module script graph fetching procedure
  // should be performed in parallel to each other. [spec text]
  for (size_t i = 0; i < urls.size(); ++i) {
    // "internal module script graph fetching procedure":

    // [IMSGF] Step 1. Fetch a single module script given url, ...
    ModuleScriptFetchRequest request(
        urls[i], module_script->Nonce(), module_script->ParserState(),
        module_script->CredentialsMode(), module_script->BaseURL().GetString(),
        positions[i]);
    FetchSingle(request, ModuleGraphLevel::kDependentModuleFetch);

    // [IMSGF] Step 2-- are executed when NotifyModuleLoadFinished() is called.
  }

  // Asynchronously continue processing after NotifyModuleLoadFinished() is
  // called num_incomplete_fetches_ times.
  CHECK_GT(num_incomplete_fetches_, 0u);
}

// Find the first error module script in the depth-first-order, and set the
// error information to |result_|.
// |reached_url_set| represents already visited nodes in the depth-first search
// by their URLs.
// Returns true if an error module is found.
//
// The depth-first search structure stems from "fetch the descendants of a
// module script" in the spec, and thus aligns with
// ModuleTreeLinker::FetchDescendants().
bool ModuleTreeLinker::PropagateError(HashSet<KURL>& reached_url_set,
                                      ModuleScript* module_script) {
  // If |module_script| (i.e. the current node in the depth-first search) is,
  // error, then propagate the error to |result_| and terminate the depth-first
  // search by returning |true|.
  //
  // This corresponds Step 7 of [FD] of the spec.

  // [FD] Step 7.1. If result is null,
  // asynchronously complete this algorithm with null, aborting these steps.
  if (!module_script) {
    result_ = nullptr;
    return true;
  }

  // [FD] Step 7.2: If result is errored, then set the pre-instantiation error
  // for module script to result's error.
  // Asynchronously complete this slgorithm with module script, aborting
  // these steps.
  if (module_script->IsErrored()) {
    result_->SetErrorAndClearRecord(modulator_->GetError(module_script));
    return true;
  }

  // Otherwise, continue the depth-first search.

  if (module_script->HasInstantiated()) {
    return false;
  }

  DCHECK_EQ(ScriptModuleState::kUninstantiated, module_script->RecordStatus());

  ScriptModule record = module_script->Record();
  DCHECK(!record.IsNull());

  Vector<Modulator::ModuleRequest> module_requests =
      modulator_->ModuleRequestsFromScriptModule(record);
  for (const auto& module_request : module_requests) {
    KURL url = Modulator::ResolveModuleSpecifier(module_request.specifier,
                                                 module_script->BaseURL());

    CHECK(url.IsValid()) << "Modulator::resolveModuleSpecifier() impl must "
                            "return either a valid url or null.";

    if (reached_url_set.Contains(url))
      continue;
    reached_url_set.insert(url);

    if (PropagateError(reached_url_set,
                       modulator_->GetFetchedModuleScript(url)))
      return true;
  }

  return false;
}

// [FDaI] Step 3--6.
void ModuleTreeLinker::Instantiate() {
  // [FDaI] Step 1-2 are "fetching the descendants of a module script", which
  // we just executed.

  // [nospec] Abort the steps if the browsing context is discarded.
  if (!modulator_->HasValidContext()) {
    result_ = nullptr;
    AdvanceState(State::kFinished);
    return;
  }

  // [Optimization] ModuleTreeLinker
  // - first fetches all descendant module scripts under the root module script
  //   without error propagation specified in [FD] Step 7, and
  // - later determines here which error should have been propagated to the root
  //   module script and propagates the error in PropagateError(),
  //   if an error is found during fetching.
  if (found_error_ && !(!result_ || result_->IsErrored())) {
    HashSet<KURL> reached_url_set;
    reached_url_set.insert(result_->BaseURL());
    PropagateError(reached_url_set, result_);
  }

  // [FDaI] Step 3. If result is null or is errored, then asynchronously
  // complete this algorithm with result.
  if (!result_ || result_->IsErrored()) {
    AdvanceState(State::kFinished);
    return;
  }

  // [FD] Step 7. If we've reached this point, all of the internal module
  // script graph fetching procedure invocations have asynchronously
  // completed, with module scripts that are not errored. Asynchronously
  // complete this algorithm with module script.

  CHECK(result_);
  AdvanceState(State::kInstantiating);

  // [FDaI] Step 4. Let record be result's module record.
  ScriptModule record = result_->Record();

  // [FDaI] Step 5. Perform record.ModuleDeclarationInstantiation().
  // "If this throws an exception, ignore it for now; it is stored as result's
  // error, and will be reported when we run result." [spec text]
  modulator_->InstantiateModule(record);

  // [FDaI] Step 6. Asynchronously complete this algorithm with result.
  AdvanceState(State::kFinished);
}

}  // namespace blink

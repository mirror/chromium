// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/modulescript/ModuleTreeLinker.h"

#include "bindings/core/v8/ScriptModule.h"
#include "core/dom/ModuleScript.h"
#include "core/loader/modulescript/ModuleScriptFetchRequest.h"
#include "core/loader/modulescript/ModuleTreeLinkerRegistry.h"
#include "core/loader/modulescript/ModuleTreeReachedUrlSet.h"
#include "platform/WebTaskRunner.h"
#include "platform/bindings/V8ThrowException.h"
#include "platform/loader/fetch/ResourceLoadingLog.h"
#include "platform/wtf/Vector.h"
#include "v8/include/v8.h"

namespace blink {

ModuleTreeLinker* ModuleTreeLinker::Fetch(
    const ModuleScriptFetchRequest& request,
    Modulator* modulator,
    ModuleTreeLinkerRegistry* registry,
    ModuleTreeClient* client) {
  ModuleTreeLinker* fetcher = new ModuleTreeLinker(modulator, registry, client);
  fetcher->FetchSelf(request);
  return fetcher;
}

ModuleTreeLinker* ModuleTreeLinker::FetchDescendantsForInlineScript(
    ModuleScript* module_script,
    Modulator* modulator,
    ModuleTreeLinkerRegistry* registry,
    ModuleTreeClient* client) {
  // Substep 4 in "module" case in Step 22 of "prepare a script":"
  // https://html.spec.whatwg.org/#prepare-a-script
  DCHECK(module_script);

  // 4. "Fetch the descendants of script (using an empty ancestor list)."
  ModuleTreeLinker* fetcher = new ModuleTreeLinker(modulator, registry, client);
  fetcher->result_ = module_script;
  fetcher->AdvanceState(State::kFetchingSelf);
  fetcher->AdvanceState(State::kFetchingDependencies);

  // "When this asynchronously completes, set the script's script to
  //  the result. At that time, the script is ready."
  //
  // Currently we execute "internal module script graph
  // fetching procedure" Step 5- in addition to "fetch the descendants",
  // which is not specced yet. https://github.com/whatwg/html/issues/2544
  // TODO(hiroshige): Fix the implementation and/or comments once the spec
  // is updated.
  modulator->TaskRunner()->PostTask(
      BLINK_FROM_HERE,
      WTF::Bind(&ModuleTreeLinker::FetchDescendants, WrapPersistent(fetcher),
                WrapPersistent(module_script)));
  return fetcher;
}

ModuleTreeLinker::ModuleTreeLinker(Modulator* modulator,
                                   ModuleTreeLinkerRegistry* registry,
                                   ModuleTreeClient* client)
    : modulator_(modulator),
      reached_url_set_(new ModuleTreeReachedUrlSet),
      registry_(registry),
      client_(client),
      result_(this, nullptr) {
  CHECK(modulator);
  CHECK(reached_url_set_);
  CHECK(registry);
  CHECK(client);
}

DEFINE_TRACE(ModuleTreeLinker) {
  visitor->Trace(modulator_);
  visitor->Trace(reached_url_set_);
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
      CHECK_EQ(num_incomplete_descendants_, 0u);
      CHECK_EQ(new_state, State::kFetchingSelf);
      break;
    case State::kFetchingSelf:
      CHECK_EQ(num_incomplete_descendants_, 0u);
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

    // https://html.spec.whatwg.org/multipage/webappapis.html#internal-module-script-graph-fetching-procedure
    // Step 6. When the appropriate algorithm asynchronously completes with
    // final result, asynchronously complete this algorithm with final result.
    client_->NotifyModuleTreeLoadFinished(result_);
  }
}

void ModuleTreeLinker::FetchSelf(const ModuleScriptFetchRequest& request) {
  // https://html.spec.whatwg.org/multipage/webappapis.html#internal-module-script-graph-fetching-procedure

  // Step 1. Fetch a single module script given url, fetch client settings
  // object, destination, cryptographic nonce, parser state, credentials mode,
  // module map settings object, referrer, and the top-level module fetch flag.
  // If the caller of this algorithm specified custom perform the fetch steps,
  // pass those along while fetching a single module script.
  AdvanceState(State::kFetchingSelf);
  reached_url_set_->ObserveModuleTreeLink(request.Url());
  ++num_incomplete_descendants_;
  modulator_->FetchSingle(request, this);

  // Step 2. Return from this algorithm, and run the following steps when
  // fetching a single module script asynchronously completes with result.
  // Note: Modulator::FetchSingle asynchronously notifies result to
  // ModuleTreeLinker::NotifyModuleLoadFinished().
}

void ModuleTreeLinker::NotifyModuleLoadFinished(ModuleScript* result) {
  // https://html.spec.whatwg.org/multipage/webappapis.html#internal-module-script-graph-fetching-procedure
  CHECK_GT(num_incomplete_descendants_, 0u);
  --num_incomplete_descendants_;

  if (result) {
    RESOURCE_LOADING_DVLOG(1)
        << "ModuleTreeLinker[" << this
        << "]::DependencyModuleClient::NotifyModuleTreeLoadFinished() with "
        << *result;
  } else {
    RESOURCE_LOADING_DVLOG(1)
        << "ModuleTreeLinker[" << this
        << "]::DependencyModuleClient::NotifyModuleTreeLoadFinished() with "
        << "nullptr.";
  }

  if (state_ == State::kFetchingSelf) {
    result_ = result;
    AdvanceState(State::kFetchingDependencies);
  } else if (state_ != State::kFetchingDependencies) {
    // We may reach here if one of the descendant failed to load, and the other
    // descendants fetches were in flight.
    return;
  }

  // Step 3. "If result is null, is errored, or has instantiated, asynchronously
  // complete this algorithm with result, and abort these steps.
  // Checks are done in FetchDescendants().

  // Step 4. Assert: result's state is "uninstantiated".
  // Checks are done in FetchDescendants().

  // Step 5. If the top-level module fetch flag is set, fetch the descendants of
  // and instantiate result given destination and an ancestor list obtained by
  // appending url to ancestor list. Otherwise, fetch the descendants of result
  // given the same arguments.
  // Note: top-level module fetch flag is checked at Instantiate(), where
  //       "fetch the descendants of and instantiate" procedure  and
  //       "fetch the descendants" procedure actually diverge.
  FetchDescendants(result);
}

void ModuleTreeLinker::FetchDescendants(ModuleScript* module_script) {
  // [nospec] Abort the steps if the browsing context is discarded.
  if (!modulator_->HasValidContext()) {
    result_ = nullptr;
    AdvanceState(State::kFinished);
    return;
  }

  // https://html.spec.whatwg.org/multipage/webappapis.html#fetch-the-descendants-of-a-module-script
  // Step 1. If ancestor list was not given, let it be the empty list.
  // Note: The "ancestor list" in spec corresponds to |ancestor_list_with_url_|.

  // Step 2. If module script is errored or has instantiated,
  // asynchronously complete this algorithm with module script, and abort these
  // steps.
  if (!module_script) {
    result_ = nullptr;
    AdvanceState(State::kFinished);
    return;
  }

  if (module_script->IsErrored()) {
    // FOXME(hiroshige): We have to fetch all descendants even after we got
    // some error submodules, to get the same error reliably.
    // If we allow to report arbitrary error, then we can early-exit here.
    ScriptValue error = modulator_->GetError(module_script);
    result_->SetErrorAndClearRecord(error);
    AdvanceState(State::kFinished);
    return;
  }

  if (module_script->HasInstantiated()) {
    // "asynchronously complete this algorithm with result, and abort these
    // steps."
    if (num_incomplete_descendants_ == 0)
      Instantiate();
    return;
  }

  DCHECK_EQ(ScriptModuleState::kUninstantiated, module_script->RecordStatus());

  // Step 3. Let record be module script's module record.
  ScriptModule record = module_script->Record();
  DCHECK(!record.IsNull());

  // Step 4. If record.[[RequestedModules]] is empty, asynchronously complete
  // this algorithm with module script.
  // Note: We defer this bail-out until the end of the procedure. The rest of
  //       the procedure will be no-op anyway if record.[[RequestedModules]]
  //       is empty.

  // Step 5. Let urls be a new empty list.
  Vector<KURL> urls;
  Vector<TextPosition> positions;

  // Step 6. For each string requested of record.[[RequestedModules]],
  Vector<Modulator::ModuleRequest> module_requests =
      modulator_->ModuleRequestsFromScriptModule(record);
  for (const auto& module_request : module_requests) {
    // Step 6.1. Let url be the result of resolving a module specifier given
    // module script and requested.
    KURL url = Modulator::ResolveModuleSpecifier(module_request.specifier,
                                                 module_script->BaseURL());

    // Step 6.2. Assert: url is never failure, because resolving a module
    // specifier must have been previously successful with these same two
    // arguments.
    CHECK(url.IsValid()) << "Modulator::resolveModuleSpecifier() impl must "
                            "return either a valid url or null.";

    // [unspec] If we already have started a sub-graph fetch for the |url| for
    // this top-level module graph starting from |top_level_linker_|, we can
    // safely rely on other module graph node ModuleTreeLinker to handle it.
    if (reached_url_set_->IsAlreadyBeingFetched(url))
      continue;

    urls.push_back(url);
    positions.push_back(module_request.position);
  }

  if (urls.IsEmpty()) {
    // Step 4. If record.[[RequestedModules]] is empty, asynchronously
    // complete this algorithm with module script. [spec text]

    // Also, if record.[[RequestedModules]] is not empty but |urls| is
    // empty here, we complete this algorithm.
    if (num_incomplete_descendants_ == 0)
      Instantiate();
    return;
  }

  // Step 7. For each url in urls, perform the internal module script graph
  // fetching procedure given url, module script's credentials mode, module
  // script's cryptographic nonce, module script's parser state, destination,
  // module script's settings object, module script's settings object, ancestor
  // list, module script's base URL, and with the top-level module fetch flag
  // unset. If the caller of this algorithm specified custom perform the fetch
  // steps, pass those along while performing the internal module script graph
  // fetching procedure.

  // Step 7, when "urls" is non-empty.
  // These invocations of the internal module script graph fetching procedure
  // should be performed in parallel to each other. [spec text]
  for (size_t i = 0; i < urls.size(); ++i) {
    reached_url_set_->ObserveModuleTreeLink(urls[i]);

    ++num_incomplete_descendants_;
    ModuleScriptFetchRequest request(
        urls[i], module_script->Nonce(), module_script->ParserState(),
        module_script->CredentialsMode(), module_script->BaseURL().GetString(),
        positions[i], true);
    modulator_->FetchSingle(request, this);
  }

  // Asynchronously continue processing after NotifyOneDescendantFinished() is
  // called num_incomplete_descendants_ times.
  CHECK_GT(num_incomplete_descendants_, 0u);
}

void ModuleTreeLinker::Instantiate() {
  CHECK(result_);
  AdvanceState(State::kInstantiating);

  // https://html.spec.whatwg.org/multipage/webappapis.html#internal-module-script-graph-fetching-procedure
  // Step 5. "If the top-level module fetch flag is set, fetch the descendants
  // of and instantiate result given destination and an ancestor list obtained
  // by appending url to ancestor list. Otherwise, fetch the descendants of
  // result given the same arguments." [spec text]

  // https://html.spec.whatwg.org/multipage/webappapis.html#fetch-the-descendants-of-and-instantiate-a-module-script
  // [nospec] Abort the steps if the browsing context is discarded.
  if (!modulator_->HasValidContext()) {
    result_ = nullptr;
    AdvanceState(State::kFinished);
    return;
  }

  // Step 1-2 are "fetching the descendants of a module script", which we just
  // executed.

  // Step 3. "If result is null or is errored, ..." [spec text]
  if (!result_ || result_->IsErrored()) {
    // "then asynchronously complete this algorithm with result." [spec text]
    result_ = nullptr;
    AdvanceState(State::kFinished);
    return;
  }

  // Step 4. "Let record be result's module record." [spec text]
  ScriptModule record = result_->Record();

  // Step 5. "Perform record.ModuleDeclarationInstantiation()." [spec text]

  // "If this throws an exception, ignore it for now; it is stored as result's
  // error, and will be reported when we run result." [spec text]
  modulator_->InstantiateModule(record);

  // Step 6. Asynchronously complete this algorithm with descendants result.
  AdvanceState(State::kFinished);
}

}  // namespace blink

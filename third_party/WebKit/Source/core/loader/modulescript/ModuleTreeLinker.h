// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ModuleTreeLinker_h
#define ModuleTreeLinker_h

#include "core/CoreExport.h"
#include "core/dom/Modulator.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/bindings/TraceWrapperMember.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/KURLHash.h"
#include "platform/wtf/HashSet.h"

namespace blink {

class ModuleScriptFetchRequest;
class ModuleTreeLinkerRegistry;

// A ModuleTreeLinker is responsible for running and keeping intermediate states
// for "internal module script graph fetching procedure" for a module graph tree
// node.
// https://html.spec.whatwg.org/multipage/webappapis.html#internal-module-script-graph-fetching-procedure
class CORE_EXPORT ModuleTreeLinker final : public SingleModuleClient {
 public:
  static ModuleTreeLinker* Fetch(const ModuleScriptFetchRequest&,
                                 Modulator*,
                                 ModuleTreeLinkerRegistry*,
                                 ModuleTreeClient*);
  static ModuleTreeLinker* FetchDescendantsForInlineScript(
      ModuleScript*,
      Modulator*,
      ModuleTreeLinkerRegistry*,
      ModuleTreeClient*);

  virtual ~ModuleTreeLinker() = default;
  DECLARE_TRACE();
  DECLARE_TRACE_WRAPPERS();

  bool IsFetching() const {
    return State::kFetchingSelf <= state_ && state_ < State::kFinished;
  }
  bool HasFinished() const { return state_ == State::kFinished; }

 private:
  ModuleTreeLinker(Modulator*, ModuleTreeLinkerRegistry*, ModuleTreeClient*);

  enum class State {
    kInitial,
    // Running fetch of the module script corresponding to the target node.
    kFetchingSelf,
    // Running fetch of descendants of the target node.
    kFetchingDependencies,
    // Instantiating module_script_ and the node descendants.
    kInstantiating,
    kFinished,
  };
#if DCHECK_IS_ON()
  static const char* StateToString(State);
#endif
  void AdvanceState(State);

  void FetchRoot(const ModuleScriptFetchRequest&);
  void FetchRootInline(ModuleScript*);

  void FetchSingle(const ModuleScriptFetchRequest&, ModuleGraphLevel);
  // Implements SingleModuleClient
  void NotifyModuleLoadFinished(ModuleScript*) override;
  void CompleteFetchDescendantsForOneModuleScript();

  void Instantiate();
  bool PropagateError(HashSet<KURL>&, ModuleScript*);

  const Member<Modulator> modulator_;
  HashSet<KURL> reached_url_set_;
  const Member<ModuleTreeLinkerRegistry> registry_;
  const Member<ModuleTreeClient> client_;
  State state_ = State::kInitial;

  // Correspond to _result_ in
  // https://html.spec.whatwg.org/multipage/webappapis.html#internal-module-script-graph-fetching-procedure
  TraceWrapperMember<ModuleScript> result_;

  bool found_error_ = false;

  size_t num_incomplete_fetches_ = 0;
};

}  // namespace blink

#endif

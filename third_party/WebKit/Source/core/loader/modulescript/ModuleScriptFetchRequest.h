// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ModuleScriptFetchRequest_h
#define ModuleScriptFetchRequest_h

#include "platform/loader/fetch/ScriptFetchOptions.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/Referrer.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

// A ModuleScriptFetchRequest essentially serves as a "parameter object" for
// Modulator::Fetch{Tree,Single,NewSingle}.
class ModuleScriptFetchRequest final {
  STACK_ALLOCATED();

 public:
  ModuleScriptFetchRequest(const KURL& url, const ScriptFetchOptions& options)
      : ModuleScriptFetchRequest(url,
                                 options,
                                 Referrer::NoReferrer(),
                                 TextPosition::MinimumPosition(),
                                 false) {}
  // For link rel=modulepreload requests
  ModuleScriptFetchRequest(const KURL& url,
                           WebURLRequest::FetchCredentialsMode credentials_mode)
      : ModuleScriptFetchRequest(
            url,
            ScriptFetchOptions(String(), kParserInserted, credentials_mode),
            Referrer::NoReferrer(),
            TextPosition::MinimumPosition(),
            true) {}
  ~ModuleScriptFetchRequest() = default;

  const KURL& Url() const { return url_; }
  const ScriptFetchOptions& Options() const { return options_; }
  const AtomicString& GetReferrer() const { return referrer_; }
  const TextPosition& GetReferrerPosition() const { return referrer_position_; }
  bool IsModulePreload() const { return is_module_preload_; }

 private:
  // Referrer is set only for internal module script fetch algorithms triggered
  // from ModuleTreeLinker to fetch descendant module scripts.
  friend class ModuleTreeLinker;
  ModuleScriptFetchRequest(const KURL& url,
                           const ScriptFetchOptions& options,
                           const String& referrer,
                           const TextPosition& referrer_position,
                           bool is_module_preload)
      : url_(url),
        options_(options),
        referrer_(referrer),
        referrer_position_(referrer_position),
        is_module_preload_(is_module_preload) {}

  const KURL url_;
  const ScriptFetchOptions options_;
  const AtomicString referrer_;
  const TextPosition referrer_position_;
  const bool is_module_preload_;
};

}  // namespace blink

#endif

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ModuleScriptFetchRequest_h
#define ModuleScriptFetchRequest_h

#include "platform/loader/fetch/ResourceLoaderOptions.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/Referrer.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/WebURLRequest.h"

namespace blink {

// A ModuleScriptFetchRequest corresponds to the spec concept
// "#script-fetch-options" + target url.
// https://html.spec.whatwg.org/multipage/webappapis.html#concept-script-fetch-options-nonce
// A ModuleScriptFetchRequest essentially serves as a "parameter object" for
// Modulator::Fetch{Tree,Single,NewSingle}.
class ModuleScriptFetchRequest final {
  STACK_ALLOCATED();

 public:
  ModuleScriptFetchRequest(const KURL& url,
                           const String& nonce,
                           ParserDisposition parser_state,
                           WebURLRequest::FetchCredentialsMode credentials_mode)
      : ModuleScriptFetchRequest(url,
                                 nonce,
                                 parser_state,
                                 credentials_mode,
                                 Referrer::NoReferrer(),
                                 TextPosition::MinimumPosition()) {}
  ~ModuleScriptFetchRequest() = default;

  const KURL& Url() const { return url_; }
  const String& Nonce() const { return nonce_; }
  const ParserDisposition& ParserState() const { return parser_state_; }
  WebURLRequest::FetchCredentialsMode CredentialsMode() const {
    return credentials_mode_;
  }
  const AtomicString& GetReferrer() const { return referrer_; }
  const TextPosition& GetReferrerPosition() const { return referrer_position_; }

 private:
  // Referrer is set only for internal module script fetch algorithms triggered
  // from ModuleTreeLinker to fetch descendant module scripts.
  friend class ModuleTreeLinker;
  ModuleScriptFetchRequest(const KURL& url,
                           const String& nonce,
                           ParserDisposition parser_state,
                           WebURLRequest::FetchCredentialsMode credentials_mode,
                           const String& referrer,
                           const TextPosition& referrer_position)
      : url_(url),
        nonce_(nonce),
        parser_state_(parser_state),
        credentials_mode_(credentials_mode),
        referrer_(referrer),
        referrer_position_(referrer_position) {}

  const KURL url_;

  // https://html.spec.whatwg.org/multipage/webappapis.html#concept-script-fetch-options-nonce
  const String nonce_;

  // https://html.spec.whatwg.org/multipage/webappapis.html#concept-script-fetch-options-parser
  const ParserDisposition parser_state_;

  // https://html.spec.whatwg.org/multipage/webappapis.html#concept-script-fetch-options-credentials
  const WebURLRequest::FetchCredentialsMode credentials_mode_;

  const AtomicString referrer_;
  const TextPosition referrer_position_;
};

}  // namespace blink

#endif

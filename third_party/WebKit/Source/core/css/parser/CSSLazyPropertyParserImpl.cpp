// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/parser/CSSLazyPropertyParserImpl.h"

#include "core/css/parser/CSSLazyParsingState.h"
#include "core/css/parser/CSSParserImpl.h"

namespace blink {

CSSLazyPropertyParserImpl::CSSLazyPropertyParserImpl(CSSLazyParsingState* state,
                                                     size_t offset)
    : CSSLazyPropertyParser(), lazy_state_(state), offset_(offset) {
  DCHECK(lazy_state_);
}

StylePropertySet* CSSLazyPropertyParserImpl::ParseProperties() {
  lazy_state_->CountRuleParsed();
  return CSSParserImpl::ParseDeclarationListForLazyStyle(
      lazy_state_->SheetText(), offset_, lazy_state_->Context());
}

}  // namespace blink

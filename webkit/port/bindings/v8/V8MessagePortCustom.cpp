// Copyright (c) 2008, Google Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "config.h"

#include "v8_binding.h"
#include "v8_custom.h"
#include "v8_events.h"
#include "v8_proxy.h"

#include "V8Document.h"
#include "V8HTMLDocument.h"

#include "ExceptionCode.h"
#include "MessagePort.h"

namespace WebCore {

ACCESSOR_GETTER(MessagePortOnmessage) {
  INC_STATS(L"DOM.MessagePort.onmessage._get");
  V8Proxy::SetDOMException(NOT_SUPPORTED_ERR);
  return v8::Undefined();
}

ACCESSOR_SETTER(MessagePortOnmessage) {
  INC_STATS(L"DOM.MessagePort.onmessage._set");
  V8Proxy::SetDOMException(NOT_SUPPORTED_ERR);
}

ACCESSOR_GETTER(MessagePortOnclose) {
  INC_STATS(L"DOM.MessagePort.onclose._get");
  V8Proxy::SetDOMException(NOT_SUPPORTED_ERR);
  return v8::Undefined();
}

ACCESSOR_SETTER(MessagePortOnclose) {
  INC_STATS(L"DOM.MessagePort.onclose._set");
  V8Proxy::SetDOMException(NOT_SUPPORTED_ERR);
}

CALLBACK_FUNC_DECL(MessagePortStartConversation) {
  INC_STATS(L"DOM.MessagePort.StartConversation()");
  V8Proxy::SetDOMException(NOT_SUPPORTED_ERR);
  return v8::Undefined();
}

CALLBACK_FUNC_DECL(MessagePortAddEventListener) {
  INC_STATS(L"DOM.MessagePort.AddEventListener()");
  V8Proxy::SetDOMException(NOT_SUPPORTED_ERR);
  return v8::Undefined();
}

CALLBACK_FUNC_DECL(MessagePortRemoveEventListener) {
  INC_STATS(L"DOM.MessagePort.RemoveEventListener()");
  V8Proxy::SetDOMException(NOT_SUPPORTED_ERR);
  return v8::Undefined();
}


}  // namespace WebCore

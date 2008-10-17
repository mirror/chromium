// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
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

// Javascript exception abstraction

#ifndef ScriptCallContext_h
#define ScriptCallContext_h

#include <wtf/Noncopyable.h>

#if USE(JSC)
#include <kjs/ArgList.h>
#include "KURL.h"

namespace KJS {
class ExecState;
}
#endif

namespace WebCore {

class KURL;

#if USE(V8)
typedef v8::Local<v8::Value> JSValueRef;
#endif

// Provides information about a script call with access to call arguments, line
// number, file and (in the future) other information about a script call.
class ScriptCallContext : Noncopyable {
public:
#if USE(V8)
    ScriptCallContext(const v8::Arguments& args);
#elif USE(JSC)
    ScriptCallContext(KJS::ExecState*,
                      const KJS::ArgList&,
                      unsigned sliceFrom = 0);
#endif
    ~ScriptCallContext() {}

    String argumentStringAt(unsigned, bool checkForNullOrUndefined = false);
#if USE(JSC)
    // TODO(dglazkov): Ideally, public interface should be VM-agnostic. We
    // should work to remove this ifdef.
    KJS::JSValue* argumentAt(unsigned);
    KJS::ExecState* exec() const { return m_exec; }
#endif
    unsigned argumentCount() const;
    bool hasArguments() const { return argumentCount() > 0; }

    unsigned lineNumber() const;
    KURL sourceURL() const;
private:
#if USE(JSC)
    KJS::ExecState* m_exec;
    KJS::ArgList m_args;
    unsigned m_lineNumber;
    KURL m_sourceURL;
#elif USE(V8)
    v8::Arguments m_args;
#endif
};

}  // namespace WebCore

#endif  // !defined(ScriptCallContext_h)

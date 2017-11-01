/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WTF_h
#define WTF_h

#include "build/build_config.h"
#include "platform/wtf/Compiler.h"
#include "platform/wtf/Threading.h"
#include "platform/wtf/WTFExport.h"

namespace WTF {

typedef void MainThreadFunction(void*);

WTF_EXPORT extern ThreadIdentifier g_main_thread_identifier;

#if defined(COMPONENT_BUILD)
WTF_EXPORT bool IsMainThread();
#else  // defined(COMPONENT_BUILD)
extern thread_local bool g_is_main_thread;
inline bool IsMainThread() {
  return g_is_main_thread;
}
#endif

// This function must be called exactly once from the main thread before using
// anything else in WTF.
WTF_EXPORT void Initialize(void (*)(MainThreadFunction, void*));

inline ThreadIdentifier GetMainThreadIdentifier() {
  return g_main_thread_identifier;
}

namespace internal {
void CallOnMainThread(MainThreadFunction*, void* context);
}  // namespace internal

}  // namespace WTF

using WTF::IsMainThread;

#endif  // WTF_h

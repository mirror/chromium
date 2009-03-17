/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#include "config.h"
#include "WebKit.h"

#include "WebString.h"

#include "AtomicString.h"
#include "FrameLoader.h"
#include "V8Proxy.h"
#include "WorkerContextExecutionProxy.h"
#include <wtf/Assertions.h>
#include <wtf/Threading.h>

namespace WebKit {

static WebKitClient* s_webKitClient = 0;
static bool s_layoutTestMode = false;

void initialize(WebKitClient* webKitClient)
{
    ASSERT(webKitClient);
    ASSERT(!s_webKitClient);
    s_webKitClient = webKitClient;

    WTF::initializeThreading();
    WebCore::AtomicString::init();
}

void shutdown()
{
    s_webKitClient = 0;
}

WebKitClient* webKitClient()
{
    return s_webKitClient;
}

void setLayoutTestMode(bool value)
{
    s_layoutTestMode = value;
}

bool layoutTestMode()
{
    return s_layoutTestMode;
}

void registerURLSchemeAsLocal(const WebString& scheme)
{
    WebCore::FrameLoader::registerURLSchemeAsLocal(scheme);
}

void registerURLSchemeAsNoAccess(const WebString& scheme)
{
    WebCore::FrameLoader::registerURLSchemeAsNoAccess(scheme);
}

void registerExtension(v8::Extension* extension)
{
    WebCore::V8Proxy::RegisterExtension(extension);
}

void enableWebWorkers(bool value)
{
#if ENABLE(WORKERS)
    WebCore::WorkerContextExecutionProxy::EnableWebWorkers(value);
#endif
}

} // namespace WebKit

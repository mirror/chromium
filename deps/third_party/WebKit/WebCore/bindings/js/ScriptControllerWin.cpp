/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "ScriptController.h"

#if PLATFORM(CHROMIUM)
#include "webkit/glue/webplugin_impl.h"
#include "c_instance.h"
#include "npruntime.h"
#else
#include "PluginView.h"
#endif
#include "runtime.h"

using namespace KJS::Bindings;

namespace WebCore {

JSInstanceHandle ScriptController::createScriptInstanceForWidget(Widget* widget)
{
    if (!widget->isPluginView())
        return 0;

#if PLATFORM(CHROMIUM)
    WebPluginContainer* container = static_cast<WebPluginContainer*>(widget);
    NPObject* object = container->GetPluginScriptableObject();
    if (!object)
        return 0;

    // Register 'widget' with the frame so that we can teardown
    // subobjects when the container goes away.
    RefPtr<KJS::Bindings::RootObject> root = m_frame->script()->createRootObject(this);
    RefPtr<KJS::Bindings::Instance> instance = KJS::Bindings::CInstance::create(object, root.release());

    // GetPluginScriptableObject returns a retained NPObject.  
    // The caller is expected to release it   
    NPN_ReleaseObject(object);
    return instance;
#else
    return static_cast<PluginView*>(widget)->bindingInstance();
#endif
}

} // namespace WebCore

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "bindings/core/v8/V8IntersectionObserver.h"

#include "bindings/core/v8/ExceptionMessages.h"
#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/V8Binding.h"
#include "bindings/core/v8/V8DOMWrapper.h"
#include "bindings/core/v8/V8GCController.h"
#include "bindings/core/v8/V8IntersectionObserverCallback.h"
#include "bindings/core/v8/V8IntersectionObserverInit.h"

namespace blink {

void V8IntersectionObserver::constructorCustom(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (UNLIKELY(info.Length() < 1)) {
        V8ThrowException::throwException(createMinimumArityTypeErrorForMethod(info.GetIsolate(), "createIntersectionObserver", "Intersection", 1, info.Length()), info.GetIsolate());
        return;
    }

    v8::Local<v8::Object> wrapper = info.Holder();

    Document* document = nullptr;
    DOMWindow* window = toDOMWindow(wrapper->CreationContext());
    if (window)
        document = window->document();

    if (info.Length() <= 0 || !info[0]->IsFunction()) {
        V8ThrowException::throwTypeError(info.GetIsolate(), ExceptionMessages::failedToExecute("createIntersectionObserver", "Intersection", "The callback provided as parameter 1 is not a function."));
        return;
    }

    if (info.Length() > 1 && !isUndefinedOrNull(info[1]) && !info[1]->IsObject()) {
        V8ThrowException::throwTypeError(info.GetIsolate(), ExceptionMessages::failedToExecute("createIntersectionObserver", "Intersection", "IntersectionObserverInit (parameter 2) is not an object."));
        return;
    }

    OwnPtrWillBeRawPtr<IntersectionObserverCallback> callback = V8IntersectionObserverCallback::create(v8::Local<v8::Function>::Cast(info[0]), wrapper, ScriptState::current(info.GetIsolate()));

    IntersectionObserverInit intersectionObserverInit;
    ExceptionState exceptionState(ExceptionState::ConstructionContext, "Intersection", info.Holder(), info.GetIsolate());
    V8IntersectionObserverInit::toImpl(info.GetIsolate(), info[1], intersectionObserverInit, exceptionState);

    RefPtrWillBeRawPtr<IntersectionObserver> observer = IntersectionObserver::create(document, intersectionObserverInit, callback.release());

    v8SetReturnValue(info, V8DOMWrapper::associateObjectWithWrapper(info.GetIsolate(), observer.get(), &wrapperTypeInfo, wrapper));
}

} // namespace blink

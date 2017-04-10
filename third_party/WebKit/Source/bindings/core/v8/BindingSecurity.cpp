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

#include "bindings/core/v8/BindingSecurity.h"

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/V8Binding.h"
#include "bindings/core/v8/V8Location.h"
#include "bindings/core/v8/V8Window.h"
#include "core/dom/Document.h"
#include "core/frame/DOMWindow.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/Location.h"
#include "core/frame/Settings.h"
#include "core/html/HTMLFrameElementBase.h"
#include "core/workers/MainThreadWorkletGlobalScope.h"
#include "platform/weborigin/SecurityOrigin.h"

namespace blink {

namespace {

bool canAccessWindowInternal(const LocalDOMWindow* accessingWindow,
                             const DOMWindow* targetWindow) {
  SECURITY_CHECK(!(targetWindow && targetWindow->frame()) ||
                 targetWindow == targetWindow->frame()->domWindow());

  // It's important to check that targetWindow is a LocalDOMWindow: it's
  // possible for a remote frame and local frame to have the same security
  // origin, depending on the model being used to allocate Frames between
  // processes. See https://crbug.com/601629.
  if (!(accessingWindow && targetWindow && targetWindow->isLocalDOMWindow()))
    return false;

  const SecurityOrigin* accessingOrigin =
      accessingWindow->document()->getSecurityOrigin();
  const LocalDOMWindow* localTargetWindow = toLocalDOMWindow(targetWindow);
  if (!accessingOrigin->canAccessCheckSuborigins(
          localTargetWindow->document()->getSecurityOrigin())) {
    return false;
  }

  // Notify the loader's client if the initial document has been accessed.
  LocalFrame* targetFrame = localTargetWindow->frame();
  if (targetFrame &&
      targetFrame->loader().stateMachine()->isDisplayingInitialEmptyDocument())
    targetFrame->loader().didAccessInitialDocument();

  return true;
}

bool canAccessWindow(const LocalDOMWindow* accessingWindow,
                     const DOMWindow* targetWindow,
                     ExceptionState& exceptionState) {
  if (canAccessWindowInternal(accessingWindow, targetWindow))
    return true;

  if (targetWindow)
    exceptionState.throwSecurityError(
        targetWindow->sanitizedCrossDomainAccessErrorMessage(accessingWindow),
        targetWindow->crossDomainAccessErrorMessage(accessingWindow));
  return false;
}

bool canAccessWindow(const LocalDOMWindow* accessingWindow,
                     const DOMWindow* targetWindow,
                     BindingSecurity::ErrorReportOption reportingOption) {
  if (canAccessWindowInternal(accessingWindow, targetWindow))
    return true;

  if (accessingWindow && targetWindow &&
      reportingOption == BindingSecurity::ErrorReportOption::Report)
    accessingWindow->printErrorMessage(
        targetWindow->crossDomainAccessErrorMessage(accessingWindow));
  return false;
}

DOMWindow* findWindow(v8::Isolate* isolate,
                      const WrapperTypeInfo* type,
                      v8::Local<v8::Object> holder) {
  if (V8Window::wrapperTypeInfo.equals(type))
    return V8Window::toImpl(holder);

  if (V8Location::wrapperTypeInfo.equals(type))
    return V8Location::toImpl(holder)->domWindow();

  // This function can handle only those types listed above.
  NOTREACHED();
  return nullptr;
}

}  // namespace

bool BindingSecurity::shouldAllowAccessTo(const LocalDOMWindow* accessingWindow,
                                          const DOMWindow* target,
                                          ExceptionState& exceptionState) {
  DCHECK(target);
  return canAccessWindow(accessingWindow, target, exceptionState);
}

bool BindingSecurity::shouldAllowAccessTo(const LocalDOMWindow* accessingWindow,
                                          const DOMWindow* target,
                                          ErrorReportOption reportingOption) {
  DCHECK(target);
  return canAccessWindow(accessingWindow, target, reportingOption);
}

bool BindingSecurity::shouldAllowAccessTo(const LocalDOMWindow* accessingWindow,
                                          const Location* target,
                                          ExceptionState& exceptionState) {
  DCHECK(target);
  return canAccessWindow(accessingWindow, target->domWindow(), exceptionState);
}

bool BindingSecurity::shouldAllowAccessTo(const LocalDOMWindow* accessingWindow,
                                          const Location* target,
                                          ErrorReportOption reportingOption) {
  DCHECK(target);
  return canAccessWindow(accessingWindow, target->domWindow(), reportingOption);
}

bool BindingSecurity::shouldAllowAccessTo(const LocalDOMWindow* accessingWindow,
                                          const Node* target,
                                          ExceptionState& exceptionState) {
  if (!target)
    return false;
  return canAccessWindow(accessingWindow, target->document().domWindow(),
                         exceptionState);
}

bool BindingSecurity::shouldAllowAccessTo(const LocalDOMWindow* accessingWindow,
                                          const Node* target,
                                          ErrorReportOption reportingOption) {
  if (!target)
    return false;
  return canAccessWindow(accessingWindow, target->document().domWindow(),
                         reportingOption);
}

bool BindingSecurity::shouldAllowAccessToFrame(
    const LocalDOMWindow* accessingWindow,
    const Frame* target,
    ExceptionState& exceptionState) {
  if (!target || !target->securityContext())
    return false;
  return canAccessWindow(accessingWindow, target->domWindow(), exceptionState);
}

bool BindingSecurity::shouldAllowAccessToFrame(
    const LocalDOMWindow* accessingWindow,
    const Frame* target,
    ErrorReportOption reportingOption) {
  if (!target || !target->securityContext())
    return false;
  return canAccessWindow(accessingWindow, target->domWindow(), reportingOption);
}

bool BindingSecurity::shouldAllowNamedAccessTo(const DOMWindow* accessingWindow,
                                               const DOMWindow* targetWindow) {
  const Frame* accessingFrame = accessingWindow->frame();
  DCHECK(accessingFrame);
  DCHECK(accessingFrame->securityContext());
  const SecurityOrigin* accessingOrigin =
      accessingFrame->securityContext()->getSecurityOrigin();

  const Frame* targetFrame = targetWindow->frame();
  DCHECK(targetFrame);
  DCHECK(targetFrame->securityContext());
  const SecurityOrigin* targetOrigin =
      targetFrame->securityContext()->getSecurityOrigin();
  SECURITY_CHECK(!(targetWindow && targetWindow->frame()) ||
                 targetWindow == targetWindow->frame()->domWindow());

  if (!accessingOrigin->canAccessCheckSuborigins(targetOrigin))
    return false;

  // Note that there is no need to call back
  // FrameLoader::didAccessInitialDocument() because |targetWindow| must be
  // a child window inside iframe or frame and it doesn't have a URL bar,
  // so there is no need to worry about URL spoofing.

  return true;
}

void BindingSecurity::failedAccessCheckFor(v8::Isolate* isolate,
                                           const WrapperTypeInfo* type,
                                           v8::Local<v8::Object> holder) {
  DOMWindow* target = findWindow(isolate, type, holder);
  // Failing to find a target means something is wrong. Failing to throw an
  // exception could be a security issue, so just crash.
  CHECK(target);

  // TODO(dcheng): Add ContextType, interface name, and property name as
  // arguments, so the generated exception can be more descriptive.
  ExceptionState exceptionState(isolate, ExceptionState::UnknownContext,
                                nullptr, nullptr);
  exceptionState.throwSecurityError(
      target->sanitizedCrossDomainAccessErrorMessage(currentDOMWindow(isolate)),
      target->crossDomainAccessErrorMessage(currentDOMWindow(isolate)));
}

}  // namespace blink

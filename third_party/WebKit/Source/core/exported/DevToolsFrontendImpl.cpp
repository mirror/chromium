/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "core/exported/DevToolsFrontendImpl.h"

#include "bindings/core/v8/ScriptController.h"
#include "bindings/core/v8/V8BindingForCore.h"
#include "bindings/core/v8/V8DevToolsHost.h"
#include "core/exported/WebViewImpl.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/WebLocalFrameImpl.h"
#include "core/inspector/DevToolsHost.h"
#include "core/page/Page.h"
#include "platform/LayoutTestSupport.h"

namespace blink {

// static
void DevToolsFrontendImpl::BindMojoRequest(
    LocalFrame* local_frame,
    mojom::blink::DevToolsFrontendAssociatedRequest request) {
  WebLocalFrameImpl* frame = WebLocalFrameImpl::FromFrame(local_frame);
  if (frame) {
    frame->SetDevToolsFrontend(std::unique_ptr<DevToolsFrontendImpl>(
        new DevToolsFrontendImpl(local_frame, std::move(request))));
  }
}

DevToolsFrontendImpl::DevToolsFrontendImpl(
    LocalFrame* frame,
    mojom::blink::DevToolsFrontendAssociatedRequest request)
    : frame_(frame), binding_(this, std::move(request)) {}

DevToolsFrontendImpl::~DevToolsFrontendImpl() {
  if (devtools_host_)
    devtools_host_->DisconnectClient();
}

void DevToolsFrontendImpl::DidClearWindowObject() {
  if (!frame_)
    return;

  if (host_) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    // Use higher limit for DevTools isolate so that it does not OOM when
    // profiling large heaps.
    isolate->IncreaseHeapLimitForDebugging();
    ScriptState* script_state = ToScriptStateForMainWorld(frame_);
    DCHECK(script_state);
    ScriptState::Scope scope(script_state);
    if (devtools_host_)
      devtools_host_->DisconnectClient();
    devtools_host_ = DevToolsHost::Create(this, frame_);
    v8::Local<v8::Object> global = script_state->GetContext()->Global();
    v8::Local<v8::Value> devtools_host_obj =
        ToV8(devtools_host_.Get(), global, script_state->GetIsolate());
    DCHECK(!devtools_host_obj.IsEmpty());
    global->Set(V8AtomicString(isolate, "DevToolsHost"), devtools_host_obj);
  }

  if (!api_script_.IsEmpty())
    frame_->GetScriptController().ExecuteScriptInMainWorld(api_script_);
}

void DevToolsFrontendImpl::SetupDevToolsFrontend(
    const String& api_script,
    mojom::blink::DevToolsFrontendHostAssociatedPtrInfo host) {
  DCHECK(!frame_ || frame_->IsMainFrame());
  api_script_ = api_script;
  host_.Bind(std::move(host));
  host_.set_connection_error_handler(ConvertToBaseCallback(WTF::Bind(
      &DevToolsFrontendImpl::DestroyOnHostGone, WTF::Unretained(this))));
  if (frame_)
    frame_->GetPage()->SetDefaultPageScaleLimits(1.f, 1.f);
}

void DevToolsFrontendImpl::SetupDevToolsExtensionAPI(
    const String& extension_api) {
  DCHECK(!frame_ || !frame_->IsMainFrame());
  api_script_ = extension_api;
}

void DevToolsFrontendImpl::SendMessageToEmbedder(const String& message) {
  if (host_)
    host_->DispatchEmbedderMessage(message);
}

bool DevToolsFrontendImpl::IsUnderTest() {
  return LayoutTestSupport::IsRunningLayoutTest();
}

void DevToolsFrontendImpl::ShowContextMenu(LocalFrame* target_frame,
                                           float x,
                                           float y,
                                           ContextMenuProvider* menu_provider) {
  WebLocalFrameImpl::FromFrame(target_frame)
      ->ViewImpl()
      ->ShowContextMenuAtPoint(x, y, menu_provider);
}

void DevToolsFrontendImpl::DestroyOnHostGone() {
  WebLocalFrameImpl* frame = WebLocalFrameImpl::FromFrame(frame_.Get());
  if (frame)
    frame->SetDevToolsFrontend(std::unique_ptr<DevToolsFrontendImpl>());
  // |this| is deleted.
}

}  // namespace blink

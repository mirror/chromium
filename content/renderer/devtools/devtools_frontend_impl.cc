// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/devtools/devtools_frontend_impl.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/web/WebDevToolsFrontend.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace content {

// static
void DevToolsFrontendImpl::CreateMojoService(
    RenderFrame* render_frame,
    mojom::DevToolsFrontendAssociatedRequest request) {
  // Self-destructs on render frame deletion.
  new DevToolsFrontendImpl(render_frame, std::move(request));
}

DevToolsFrontendImpl::DevToolsFrontendImpl(
    RenderFrame* render_frame,
    mojom::DevToolsFrontendAssociatedRequest request)
    : RenderFrameObserver(render_frame),
      binding_(this, std::move(request)),
      weak_factory_(this) {}

DevToolsFrontendImpl::~DevToolsFrontendImpl() {}

void DevToolsFrontendImpl::DidClearWindowObject() {
  if (!api_script_.empty()) {
    // Postpone ExecuteJavaScript to make sure it executes *after* sending the
    // DidCommitProvisionalLoad IPC back to the browser (i.e. after the browser
    // is aware of the new origin the scripts should be able to access).
    GURL document_url = render_frame()->GetWebFrame()->GetDocument().Url();
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&DevToolsFrontendImpl::ExecuteApiScript,
                   weak_factory_.GetWeakPtr(), document_url, api_script_));
  }
}

void DevToolsFrontendImpl::ExecuteApiScript(const GURL& expected_url,
                                            const std::string& api_script) {
  // Verify that the script runs in the same document as the one at the time
  // when ExecuteApiScript task was created.
  GURL document_url = render_frame()->GetWebFrame()->GetDocument().Url();
  if (document_url != expected_url)
    return;

  // Execute |api_script|.
  render_frame()->ExecuteJavaScript(base::UTF8ToUTF16(api_script));
}

void DevToolsFrontendImpl::OnDestruct() {
  delete this;
}

void DevToolsFrontendImpl::SendMessageToEmbedder(
    const blink::WebString& message) {
  if (host_)
    host_->DispatchEmbedderMessage(message.Utf8());
}

bool DevToolsFrontendImpl::IsUnderTest() {
  return RenderThreadImpl::current()->layout_test_mode();
}

void DevToolsFrontendImpl::SetupDevToolsFrontend(
    const std::string& api_script,
    mojom::DevToolsFrontendHostAssociatedPtrInfo host) {
  DCHECK(render_frame()->IsMainFrame());
  api_script_ = api_script;
  web_devtools_frontend_.reset(
      blink::WebDevToolsFrontend::Create(render_frame()->GetWebFrame(), this));
  host_.Bind(std::move(host));
  host_.set_connection_error_handler(base::BindOnce(
      &DevToolsFrontendImpl::OnDestruct, base::Unretained(this)));
}

void DevToolsFrontendImpl::SetupDevToolsExtensionAPI(
    const std::string& extension_api) {
  DCHECK(!render_frame()->IsMainFrame());
  api_script_ = extension_api;
}

}  // namespace content

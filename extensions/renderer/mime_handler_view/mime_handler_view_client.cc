// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/mime_handler_view/mime_handler_view_client.h"

#include "content/public/renderer/render_frame.h"
#include "extensions/renderer/mime_handler_view/mime_handler_view_manager.h"
#include "gin/arguments.h"
#include "gin/dictionary.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/web/WebAssociatedURLLoader.h"
#include "third_party/WebKit/public/web/WebAssociatedURLLoaderOptions.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace extensions {

namespace {

const char kPostMessageName[] = "postMessage";
using PostMessageCallback =
    base::Callback<void(v8::Isolate*, v8::Local<v8::Value>)>;
// The gin-backed scriptable object which is exposed by the BrowserPlugin for
// MimeHandlerViewContainer. This currently only implements "postMessage".
class ScriptableObject : public gin::Wrappable<ScriptableObject>,
                         public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static v8::Local<v8::Object> Create(v8::Isolate* isolate,
                                      const PostMessageCallback& callback) {
    ScriptableObject* scriptable_object =
        new ScriptableObject(isolate, callback);
    return gin::CreateHandle(isolate, scriptable_object)
        .ToV8()
        .As<v8::Object>();
  }

  // gin::NamedPropertyInterceptor
  v8::Local<v8::Value> GetNamedProperty(
      v8::Isolate* isolate,
      const std::string& identifier) override {
    if (identifier == kPostMessageName) {
      if (post_message_function_template_.IsEmpty()) {
        post_message_function_template_.Reset(
            isolate, gin::CreateFunctionTemplate(
                         isolate, base::Bind(post_message_callback_, isolate)));
      }
      v8::Local<v8::FunctionTemplate> function_template =
          v8::Local<v8::FunctionTemplate>::New(isolate,
                                               post_message_function_template_);
      v8::Local<v8::Function> function;
      if (function_template->GetFunction(isolate->GetCurrentContext())
              .ToLocal(&function))
        return function;
    }
    return v8::Local<v8::Value>();
  }

 private:
  ScriptableObject(v8::Isolate* isolate, const PostMessageCallback& callback)
      : gin::NamedPropertyInterceptor(isolate, this),
        post_message_callback_(callback) {}

  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    return gin::Wrappable<ScriptableObject>::GetObjectTemplateBuilder(isolate)
        .AddNamedPropertyInterceptor();
  }

  PostMessageCallback post_message_callback_;
  v8::Persistent<v8::FunctionTemplate> post_message_function_template_;
};

// static
gin::WrapperInfo ScriptableObject::kWrapperInfo = {gin::kEmbedderNativeGin};

}  // namespace

// TODO(ekaramad): Do we need to process and validate the URL here?
MimeHandlerViewClient::MimeHandlerViewClient(
    content::RenderFrame* navigating_render_frame,
    const GURL& pdf_url,
    bool send_request)
    : content::RenderFrameObserver(navigating_render_frame),
      original_frame_routing_id_(navigating_render_frame->GetRoutingID()),
      pdf_url_(pdf_url),
      mime_handler_view_manager_(MimeHandlerViewManager::FromRenderFrame(
          content::RenderFrame::FromWebFrame(
              navigating_render_frame->GetWebFrame()->Parent()))),
      weak_factory_(this) {
  DCHECK(mime_handler_view_manager_);
  if (send_request)
    SendRequest();
}

MimeHandlerViewClient::~MimeHandlerViewClient() {}

void MimeHandlerViewClient::DocumentLoadedInFrame() {
  // Pump the messages.
}

v8::Local<v8::Object> MimeHandlerViewClient::GetV8ScriptingObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> global_proxy) {
  global_proxy_ = global_proxy;
  if (scriptable_object_.IsEmpty()) {
    PostMessageCallback callback = base::Bind(
        &MimeHandlerViewClient::PostMessage, weak_factory_.GetWeakPtr());
    v8::Local<v8::Object> object = ScriptableObject::Create(isolate, callback);
    scriptable_object_.Reset(isolate, object);
  }
  return v8::Local<v8::Object>();
}

void MimeHandlerViewClient::DidFinishLoading(double time) {
  loader_.reset(nullptr);
  resource_request_complete_ = true;
}

void MimeHandlerViewClient::OnDestruct() {
  if (loader_) {
    loader_->Cancel();
    loader_.reset();
  }
  mime_handler_view_manager_->ClientRenderFrameGone(original_frame_routing_id_);
}

void MimeHandlerViewClient::SendRequest() {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  blink::WebAssociatedURLLoaderOptions options;
  // The embedded plugin is allowed to be cross-origin and we should always
  // send credentials/cookies with the request.
  options.fetch_request_mode = blink::WebURLRequest::kFetchRequestModeNoCORS;
  options.allow_credentials = true;
  DCHECK(!loader_);
  loader_.reset(frame->CreateAssociatedURLLoader(options));

  blink::WebURLRequest request(pdf_url_);
  request.SetRequestContext(blink::WebURLRequest::kRequestContextObject);
  loader_->LoadAsynchronously(request, this);
}

void MimeHandlerViewClient::PostMessage(v8::Isolate* isolate,
                                        v8::Local<v8::Value> message) {
  if (!document_loaded_in_main_frame_) {
    pending_messages_.push_back(v8::Global<v8::Value>(isolate, message));
    return;
  }

  v8::Context::Scope context_scope(mime_handler_view_manager_->render_frame()
                                       ->GetWebFrame()
                                       ->MainWorldScriptContext());

  gin::Dictionary window_object(isolate, global_proxy_);
  v8::Local<v8::Function> post_message;
  if (!window_object.Get(std::string(kPostMessageName), &post_message))
    return;

  v8::Local<v8::Value> args[] = {
      message,
      // Post the message to any domain inside the browser plugin. The embedder
      // should already know what is embedded.
      gin::StringToV8(isolate, "*")};
  render_frame()->GetWebFrame()->CallFunctionEvenIfScriptDisabled(
      post_message.As<v8::Function>(), global_proxy_, arraysize(args), args);
}

}  // namespace extensions.

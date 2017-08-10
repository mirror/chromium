// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/mime_handler_view/mime_handler_view_frame_controller.h"

#include "base/threading/thread_task_runner_handle.h"
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
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "url/gurl.h"

namespace extensions {
namespace {

const char kPostMessageName[] = "postMessage";

// The gin-backed scriptable object which implements "postMessage".
class ScriptableObject : public gin::Wrappable<ScriptableObject>,
                         public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static v8::Local<v8::Object> Create(
      v8::Isolate* isolate,
      base::WeakPtr<MimeHandlerViewFrameController> controller) {
    ScriptableObject* scriptable_object =
        new ScriptableObject(isolate, controller);
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
            isolate,
            gin::CreateFunctionTemplate(
                isolate,
                base::Bind(&MimeHandlerViewFrameController::PostMessage,
                           controller_, isolate)));
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
  ScriptableObject(v8::Isolate* isolate,
                   base::WeakPtr<MimeHandlerViewFrameController> controller)
      : gin::NamedPropertyInterceptor(isolate, this), controller_(controller) {}

  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    return gin::Wrappable<ScriptableObject>::GetObjectTemplateBuilder(isolate)
        .AddNamedPropertyInterceptor();
  }

  base::WeakPtr<MimeHandlerViewFrameController> controller_;
  v8::Persistent<v8::FunctionTemplate> post_message_function_template_;
};

// static
gin::WrapperInfo ScriptableObject::kWrapperInfo = {gin::kEmbedderNativeGin};
}  // namespace

MimeHandlerViewFrameController::MimeHandlerViewFrameController(
    MimeHandlerViewManager* manager)
    : manager_(manager), weak_factory_(this) {
  DCHECK(manager);
}

MimeHandlerViewFrameController::~MimeHandlerViewFrameController() {}

void MimeHandlerViewFrameController::RequestResource(
    const std::string& resource_url) {
  view_id_.clear();

  blink::WebLocalFrame* frame = manager_->render_frame()->GetWebFrame();
  blink::WebAssociatedURLLoaderOptions options;
  if (loader_) {
    loader_->Cancel();
    loader_.reset(nullptr);
  }
  loader_.reset(frame->CreateAssociatedURLLoader(options));

  GURL url(resource_url);
  blink::WebURLRequest request(url);
  request.SetRequestContext(blink::WebURLRequest::kRequestContextObject);
  loader_->LoadAsynchronously(request, this);
}

void MimeHandlerViewFrameController::SetId(int32_t unique_id) {
  unique_id_ = unique_id;
}

void MimeHandlerViewFrameController::DocumentLoaded() {
  if (loader_)
    loader_.reset(nullptr);
  document_loaded_ = true;

  if (pending_messages_.empty())
    return;

  // Now that the guest has loaded, flush any unsent messages.
  blink::WebLocalFrame* frame = manager_->render_frame()->GetWebFrame();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(frame->MainWorldScriptContext());
  for (const auto& pending_message : pending_messages_)
    PostMessage(isolate, v8::Local<v8::Value>::New(isolate, pending_message));

  pending_messages_.clear();
}

v8::Local<v8::Object>
MimeHandlerViewFrameController::GetV8ScriptableObjectForPluginFrame(
    v8::Isolate* isolate) {
  if (scriptable_object_.IsEmpty()) {
    v8::Local<v8::Object> object =
        ScriptableObject::Create(isolate, weak_factory_.GetWeakPtr());
    scriptable_object_.Reset(isolate, object);
  }
  return v8::Local<v8::Object>::New(isolate, scriptable_object_);
}

// Post a JavaScript message to the guest.
void MimeHandlerViewFrameController::PostMessage(v8::Isolate* isolate,
                                                 v8::Local<v8::Value> message) {
  if (!document_loaded_) {
    pending_messages_.push_back(v8::Global<v8::Value>(isolate, message));
    return;
  }

  v8::Local<v8::Object> window_proxy = web_frame_->GlobalProxy();
  gin::Dictionary window_object(isolate, window_proxy);
  v8::Local<v8::Function> post_message;
  if (!window_object.Get(std::string(kPostMessageName), &post_message))
    return;

  v8::Local<v8::Value> args[] = {
      message,
      // Post the message to any domain inside the browser plugin. The embedder
      // should already know what is embedded.
      gin::StringToV8(isolate, "*")};
  manager_->render_frame()->GetWebFrame()->CallFunctionEvenIfScriptDisabled(
      post_message.As<v8::Function>(), window_proxy, arraysize(args), args);
}

void MimeHandlerViewFrameController::DidReceiveData(const char* data,
                                                    int data_length) {
  view_id_ += data;
}

void MimeHandlerViewFrameController::DidFinishLoading(double finish_time) {
  manager_->SetViewId(unique_id_, view_id_);
  loader_.reset(nullptr);
}

}  // namespace extensions

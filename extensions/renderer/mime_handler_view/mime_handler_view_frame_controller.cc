// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/mime_handler_view/mime_handler_view_frame_controller.h"

#include "base/threading/thread_task_runner_handle.h"
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
const int32_t kFrameTreeNodeIdNone = -1;

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
    blink::WebMimeHandlerViewManager::HandlerId id,
    const GURL& resource_url,
    MimeHandlerViewManager* manager)
    : id_(id),
      frame_tree_node_id_(kFrameTreeNodeIdNone),
      resource_url_(resource_url),
      manager_(manager),
      weak_factory_(this) {
  DCHECK(manager);
  DCHECK_NE(blink::WebMimeHandlerViewManager::kHandlerIdNone, id_);
}

MimeHandlerViewFrameController::MimeHandlerViewFrameController(
    MimeHandlerViewManager* manager)
    : manager_(manager), weak_factory_(this) {
  DCHECK(manager);
}

MimeHandlerViewFrameController::~MimeHandlerViewFrameController() {}

void MimeHandlerViewFrameController::DidPauseNavigation(
    int32_t frame_tree_node_id) {
  DCHECK(frame_tree_node_id_ == kFrameTreeNodeIdNone ||
         frame_tree_node_id_ == frame_tree_node_id);
  frame_tree_node_id_ = frame_tree_node_id;
  if (did_receive_view_id_)
    manager_->DidReceiveViewId(id_);
}

void MimeHandlerViewFrameController::RequestResource() {
  view_id_.clear();
  blink::WebLocalFrame* frame = manager_->owner_frame();
  blink::WebAssociatedURLLoaderOptions options;
  DCHECK(!loader_);
  loader_.reset(frame->CreateAssociatedURLLoader(options));
  blink::WebURLRequest request(resource_url_);
  request.SetRequestContext(blink::WebURLRequest::kRequestContextObject);
  loader_->LoadAsynchronously(request, this);
}

void MimeHandlerViewFrameController::SetId(int32_t id) {
  id_ = id;
}

void MimeHandlerViewFrameController::DocumentLoaded() {
  if (loader_)
    loader_.reset(nullptr);
  document_loaded_ = true;

  if (pending_messages_.empty())
    return;

  // Now that the guest has loaded, flush any unsent messages.
  blink::WebLocalFrame* frame = manager_->owner_frame();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(frame->MainWorldScriptContext());
  for (const auto& pending_message : pending_messages_)
    PostMessage(isolate, v8::Local<v8::Value>::New(isolate, pending_message));

  pending_messages_.clear();
}

v8::Local<v8::Object> MimeHandlerViewFrameController::V8ScriptableObject(
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

  v8::Local<v8::Object> window_proxy =
      manager_->GetContentFrameFromId(id_)->GlobalProxy();
  gin::Dictionary window_object(isolate, window_proxy);
  v8::Local<v8::Function> post_message;
  if (!window_object.Get(std::string(kPostMessageName), &post_message))
    return;

  v8::Local<v8::Value> args[] = {message,
                                 // Post the message to any domain inside the
                                 // browser plugin. The embedder should
                                 // already know what is embedded.
                                 gin::StringToV8(isolate, "*")};
  manager_->owner_frame()->CallFunctionEvenIfScriptDisabled(
      post_message.As<v8::Function>(), window_proxy, arraysize(args), args);
}

void MimeHandlerViewFrameController::DidReceiveData(const char* data,
                                                    int data_length) {
  view_id_ += data;
}

void MimeHandlerViewFrameController::DidFinishLoading(double finish_time) {
  did_receive_view_id_ = true;
  if (frame_tree_node_id_ != kFrameTreeNodeIdNone)
    manager_->DidReceiveViewId(id_);
  if (loader_)
    loader_.reset(nullptr);
}

void MimeHandlerViewFrameController::DidFail(const blink::WebURLError&) {
  manager_->DidFailLoadingResource(id_);
}

}  // namespace extensions

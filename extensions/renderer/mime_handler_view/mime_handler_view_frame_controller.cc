// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/mime_handler_view/mime_handler_view_frame_controller.h"

#include "base/threading/thread_task_runner_handle.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/common/constants.h"
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
    MimeHandlerViewManager* manager,
    const blink::WebElement& owner_element,
    const GURL& resource_url)
    : manager_(manager),
      owner_element_(owner_element),
      original_render_frame_routing_id_(MSG_ROUTING_NONE),
      resource_url_(resource_url),
      weak_factory_(this) {
  DCHECK(manager);
  DCHECK(!owner_element.IsNull());
}

MimeHandlerViewFrameController::~MimeHandlerViewFrameController() {}

void MimeHandlerViewFrameController::DidReceiveData(const char* data,
                                                    int data_length) {
  view_id_ += data;
}

void MimeHandlerViewFrameController::DidFinishLoading(double finish_time) {
  if (view_id_.empty())
    return;

  if (auto* frame = GetFrame()) {
    original_render_frame_routing_id_ =
        content::RenderFrame::FromWebFrame(frame->ToWebLocalFrame())
            ->GetRoutingID();
    manager_->DidReceiveViewId(this);
  } else {
    manager_->DidFailLoadingResource(this);
  }

  if (loader_)
    loader_.reset(nullptr);
}

void MimeHandlerViewFrameController::DidFail(const blink::WebURLError&) {
  manager_->DidFailLoadingResource(this);
}

void MimeHandlerViewFrameController::RequestResource() {
  view_id_.clear();
  blink::WebAssociatedURLLoaderOptions options;
  DCHECK(!loader_);
  loader_.reset(
      manager_->render_frame()->GetWebFrame()->CreateAssociatedURLLoader(
          options));
  blink::WebURLRequest request(resource_url_);
  request.SetRequestContext(blink::WebURLRequest::kRequestContextObject);
  loader_->LoadAsynchronously(request, this);
}

void MimeHandlerViewFrameController::StartLoading() {
  GetFrame()->ToWebLocalFrame()->Load(
      blink::WebURLRequest(blink::WebURL(GetCompletedUrl())));
}

void MimeHandlerViewFrameController::DocumentLoaded() {
  if (loader_)
    loader_.reset(nullptr);
  document_loaded_ = true;

  if (pending_messages_.empty())
    return;

  // Now that the extension has loaded, flush any unsent messages.
  blink::WebLocalFrame* frame = manager_->render_frame()->GetWebFrame();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(frame->MainWorldScriptContext());
  for (const auto& pending_message : pending_messages_)
    PostMessage(isolate, v8::Local<v8::Value>::New(isolate, pending_message));

  pending_messages_.clear();
}

v8::Local<v8::Object> MimeHandlerViewFrameController::GetV8ScriptableObject(
    v8::Isolate* isolate) {
  if (scriptable_object_.IsEmpty()) {
    v8::Local<v8::Object> object =
        ScriptableObject::Create(isolate, weak_factory_.GetWeakPtr());
    scriptable_object_.Reset(isolate, object);
  }
  return v8::Local<v8::Object>::New(isolate, scriptable_object_);
}

// Post a JavaScript message to the extension.
void MimeHandlerViewFrameController::PostMessage(v8::Isolate* isolate,
                                                 v8::Local<v8::Value> message) {
  if (!document_loaded_) {
    pending_messages_.push_back(v8::Global<v8::Value>(isolate, message));
    return;
  }

  v8::Local<v8::Object> window_proxy = GetFrame()->GlobalProxy();
  gin::Dictionary window_object(isolate, window_proxy);
  v8::Local<v8::Function> post_message;
  if (!window_object.Get(std::string(kPostMessageName), &post_message))
    return;

  v8::Local<v8::Value> args[] = {message,
                                 // Post the message to any domain inside the
                                 // browser plugin. The embedder should
                                 // already know what is embedded.
                                 gin::StringToV8(isolate, "*")};
  manager_->render_frame()->GetWebFrame()->CallFunctionEvenIfScriptDisabled(
      post_message.As<v8::Function>(), window_proxy, arraysize(args), args);
}

blink::WebFrame* MimeHandlerViewFrameController::GetFrame() const {
  return blink::WebFrame::FromFrameOwnerElement(owner_element_);
}

GURL MimeHandlerViewFrameController::GetCompletedUrl() const {
  return GURL(base::StringPrintf("%s://%s/index.html?%s", kExtensionScheme,
                                 extension_misc::kPdfExtensionId,
                                 resource_url_.spec().c_str()));
}

}  // namespace extensions

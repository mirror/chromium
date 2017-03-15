// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/api_binding_js_util.h"

#include "base/values.h"
#include "extensions/renderer/api_event_handler.h"
#include "extensions/renderer/api_request_handler.h"
#include "extensions/renderer/api_signature.h"
#include "extensions/renderer/api_type_reference_map.h"
#include "gin/converter.h"
#include "gin/object_template_builder.h"

namespace extensions {

gin::WrapperInfo APIBindingJSUtil::kWrapperInfo = {gin::kEmbedderNativeGin};

APIBindingJSUtil::APIBindingJSUtil(const APITypeReferenceMap* type_refs,
                                   APIRequestHandler* request_handler,
                                   APIEventHandler* event_handler)
    : type_refs_(type_refs),
      request_handler_(request_handler),
      event_handler_(event_handler) {}

APIBindingJSUtil::~APIBindingJSUtil() {}

gin::ObjectTemplateBuilder APIBindingJSUtil::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<APIBindingJSUtil>::GetObjectTemplateBuilder(isolate)
      .SetMethod("sendRequest", &APIBindingJSUtil::SendRequest)
      .SetMethod("registerEventArgumentMassager",
                 &APIBindingJSUtil::RegisterEventArgumentMassager)
      .SetMethod("createCustomEvent", &APIBindingJSUtil::CreateCustomEvent)
      .SetMethod("invalidateEvent", &APIBindingJSUtil::InvalidateEvent);
}

void APIBindingJSUtil::SendRequest(
    gin::Arguments* arguments,
    const std::string& name,
    const std::vector<v8::Local<v8::Value>>& request_args) {
  v8::Isolate* isolate = arguments->isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> holder;
  CHECK(arguments->GetHolder(&holder));
  v8::Local<v8::Context> context = holder->CreationContext();

  const APISignature* signature = type_refs_->GetAPIMethodSignature(name);
  DCHECK(signature);

  std::unique_ptr<base::ListValue> converted_arguments;
  v8::Local<v8::Function> callback;
  std::string error;
  CHECK(signature->ParseArgumentsToJSON(context, request_args, *type_refs_,
                                        &converted_arguments, &callback,
                                        &error));

  // TODO(devlin): The JS version of this allows callers to curry in
  // arguments, including which thread the request should be for and an
  // optional custom callback.
  request_handler_->StartRequest(context, name, std::move(converted_arguments),
                                 callback, v8::Local<v8::Function>());
}

void APIBindingJSUtil::RegisterEventArgumentMassager(
    gin::Arguments* arguments,
    const std::string& event_name,
    v8::Local<v8::Function> massager) {
  v8::Isolate* isolate = arguments->isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> holder;
  CHECK(arguments->GetHolder(&holder));
  v8::Local<v8::Context> context = holder->CreationContext();

  event_handler_->RegisterArgumentMassager(context, event_name, massager);
}

void APIBindingJSUtil::CreateCustomEvent(
    gin::Arguments* arguments,
    v8::Local<v8::Value> v8_event_name,
    v8::Local<v8::Value> unused_schema,
    v8::Local<v8::Value> unused_event_options) {
  v8::Isolate* isolate = arguments->isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> holder;
  CHECK(arguments->GetHolder(&holder));
  v8::Local<v8::Context> context = holder->CreationContext();

  std::string event_name;
  if (!v8_event_name->IsUndefined()) {
    if (!v8_event_name->IsString()) {
      NOTREACHED();
      return;
    }
    event_name = gin::V8ToString(v8_event_name);
  }

  v8::Local<v8::Value> event;
  if (event_name.empty())
    event = event_handler_->CreateAnonymousEventInstance(context);
  else
    event = event_handler_->CreateEventInstance(event_name, context);

  arguments->Return(event);
}

void APIBindingJSUtil::InvalidateEvent(gin::Arguments* arguments,
                                       v8::Local<v8::Object> event) {
  v8::Isolate* isolate = arguments->isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> holder;
  CHECK(arguments->GetHolder(&holder));
  v8::Local<v8::Context> context = holder->CreationContext();
  event_handler_->InvalidateCustomEvent(context, event);
}

}  // namespace extensions

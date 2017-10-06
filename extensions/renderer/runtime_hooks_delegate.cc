// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/runtime_hooks_delegate.h"

#include "base/strings/stringprintf.h"
#include "content/public/child/v8_value_converter.h"
#include "extensions/common/api/messaging/message.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest.h"
#include "extensions/renderer/bindings/api_signature.h"
#include "extensions/renderer/messaging_util.h"
#include "extensions/renderer/native_renderer_messaging_service.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/script_context_set.h"
#include "gin/converter.h"

namespace extensions {

namespace {
using RequestResult = APIBindingHooks::RequestResult;

constexpr char kExtensionIdRequiredErrorTemplate[] =
    "chrome.runtime.%s() called from a webpage must "
    "specify an Extension ID (string) for its first argument.";

bool GetTarget(ScriptContext* script_context,
               v8::Local<v8::Value> v8_target_id,
               std::string* target_out) {
  DCHECK(!v8_target_id.IsEmpty());

  std::string target_id;
  if (v8_target_id->IsNull()) {
    if (!script_context->extension())
      return false;

    *target_out = script_context->extension()->id();
  } else {
    DCHECK(v8_target_id->IsString());
    *target_out = gin::V8ToString(v8_target_id);
  }

  return true;
}

enum ParseParamsResult {
  TYPE_ERROR,
  THROWN,
  SUCCESS,
};

struct MessageOptions {
  std::string channel_name;
  bool include_tls_channel_id = false;
};

// Parses the parameters sent to sendMessage or connect, returning the result of
// the attempted parse. If |check_for_channel_name| is true, also checks for a
// provided channel name (this is only true for connect() calls). Populates the
// result in |params_out| or |error_out| (depending on the success of the
// parse).
ParseParamsResult ParseMessageOptions(v8::Local<v8::Context> context,
                                      v8::Local<v8::Object> v8_options,
                                      bool check_for_channel_name,
                                      MessageOptions* params_out,
                                      std::string* error_out) {
  DCHECK(!v8_options.IsEmpty());
  DCHECK(!v8_options->IsNull());

  v8::Isolate* isolate = context->GetIsolate();

  std::string channel_name;
  bool include_tls_channel_id = false;

  // Theoretically, our argument matching code already checked the types of
  // the properties on v8_connect_options. However, since we don't make an
  // independent copy, it's possible that author script has super sneaky
  // getters/setters that change the result each time the property is
  // queried. Make no assumptions.
  v8::TryCatch try_catch(isolate);
  v8::Local<v8::Value> v8_channel_name;
  v8::Local<v8::Value> v8_include_tls_channel_id;
  if (!v8_options
           ->Get(context, gin::StringToSymbol(isolate, "includeTlsChannelId"))
           .ToLocal(&v8_include_tls_channel_id) ||
      (check_for_channel_name &&
       !v8_options->Get(context, gin::StringToSymbol(isolate, "name"))
            .ToLocal(&v8_channel_name))) {
    try_catch.ReThrow();
    return THROWN;
  }

  if (check_for_channel_name && !v8_channel_name->IsUndefined()) {
    if (!v8_channel_name->IsString()) {
      *error_out = "connectInfo.name must be a string.";
      return TYPE_ERROR;
    }
    channel_name = gin::V8ToString(v8_channel_name);
  }

  if (!v8_include_tls_channel_id->IsUndefined()) {
    if (!v8_include_tls_channel_id->IsBoolean()) {
      *error_out = "connectInfo.includeTlsChannelId must be a boolean.";
      return TYPE_ERROR;
    }
    include_tls_channel_id = v8_include_tls_channel_id->BooleanValue();
  }

  params_out->channel_name = std::move(channel_name);
  params_out->include_tls_channel_id = include_tls_channel_id;
  return SUCCESS;
}

// Massages the sendMessage() arguments into the expected schema. These
// arguments are ambiguous (could match multiple signatures), so we can't just
// rely on the normal signature parsing. Sets |arguments| to the result if
// successful; otherwise leaves |arguments| untouched.
void MassageSendMessageArguments(v8::Isolate* isolate,
                                 std::vector<v8::Local<v8::Value>>* arguments) {
  if (arguments->empty() || arguments->size() > 4u)
    return;

  v8::Local<v8::Value> response_callback;
  int last_index = arguments->size() - 1;  // at most 3.
  if (arguments->at(last_index)->IsFunction()) {
    response_callback = arguments->at(last_index);
    --last_index;
  } else if (last_index == 3) {
    // The caller provided 4 arguments, but the last wasn't the callback. Abort.
    return;
  } else {
    response_callback = v8::Null(isolate);
  }

  v8::Local<v8::Value> options;
  // The first argument provided could possibly be the target extension id if
  // it's a string.
  bool first_arg_can_be_id = arguments->at(0)->IsString();
  // At this point, last_index is at most 2. If it is 2, then |options| must be
  // included. Otherwise, we have exactly two arguments left, which could be
  // either the id and the message or the message and options (since message is
  // required). We let the id have priority over options, since it's more common
  // and was defined first.
  if (last_index == 2 || (last_index == 1 && !first_arg_can_be_id)) {
    options = arguments->at(last_index);
    --last_index;
  } else {
    options = v8::Null(isolate);
  }

  // Message is required.
  v8::Local<v8::Value> message = arguments->at(last_index);
  --last_index;

  DCHECK_LE(last_index, 0);
  v8::Local<v8::Value> target_id;
  if (last_index == 0)
    target_id = arguments->at(0);
  else
    target_id = v8::Null(isolate);

  *arguments = {target_id, message, options, response_callback};
}

// Handler for the extensionId property on chrome.runtime.
void GetExtensionId(v8::Local<v8::Name> property_name,
                    const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = info.Holder()->CreationContext();

  ScriptContext* script_context =
      ScriptContextSet::GetContextByV8Context(context);
  if (script_context && script_context->extension()) {
    info.GetReturnValue().Set(
        gin::StringToSymbol(isolate, script_context->extension()->id()));
  }
}

constexpr char kGetManifest[] = "runtime.getManifest";
constexpr char kGetURL[] = "runtime.getURL";
constexpr char kConnect[] = "runtime.connect";
constexpr char kSendMessage[] = "runtime.sendMessage";

constexpr char kSendMessageChannel[] = "chrome.runtime.sendMessage";

}  // namespace

RuntimeHooksDelegate::RuntimeHooksDelegate(
    NativeRendererMessagingService* messaging_service)
    : messaging_service_(messaging_service) {}
RuntimeHooksDelegate::~RuntimeHooksDelegate() {}

RequestResult RuntimeHooksDelegate::HandleRequest(
    const std::string& method_name,
    const APISignature* signature,
    v8::Local<v8::Context> context,
    std::vector<v8::Local<v8::Value>>* arguments,
    const APITypeReferenceMap& refs) {
  ScriptContext* script_context =
      ScriptContextSet::GetContextByV8Context(context);
  DCHECK(script_context);

  if (method_name != kGetManifest && method_name != kGetURL &&
      method_name != kConnect && method_name != kSendMessage) {
    return RequestResult(RequestResult::NOT_HANDLED);
  }

  if (method_name == kSendMessage)
    MassageSendMessageArguments(context->GetIsolate(), arguments);

  std::string error;
  std::vector<v8::Local<v8::Value>> parsed_arguments;
  if (!signature->ParseArgumentsToV8(context, *arguments, refs,
                                     &parsed_arguments, &error)) {
    RequestResult result(RequestResult::INVALID_INVOCATION);
    result.error = std::move(error);
    return result;
  }

  if (method_name == kGetManifest)
    return HandleGetManifest(script_context);
  if (method_name == kGetURL)
    return HandleGetURL(script_context, parsed_arguments);
  if (method_name == kSendMessage)
    return HandleSendMessage(script_context, parsed_arguments);

  DCHECK_EQ(method_name, kConnect);
  return HandleConnect(script_context, parsed_arguments);
}

void RuntimeHooksDelegate::InitializeTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> object_template,
    const APITypeReferenceMap& type_refs) {
  object_template->SetAccessor(gin::StringToSymbol(isolate, "id"),
                               &GetExtensionId);
}

RequestResult RuntimeHooksDelegate::HandleGetManifest(
    ScriptContext* script_context) {
  DCHECK(script_context->extension());

  RequestResult result(RequestResult::HANDLED);
  result.return_value = content::V8ValueConverter::Create()->ToV8Value(
      script_context->extension()->manifest()->value(),
      script_context->v8_context());

  return result;
}

RequestResult RuntimeHooksDelegate::HandleGetURL(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  DCHECK_EQ(1u, arguments.size());
  DCHECK(arguments[0]->IsString());
  DCHECK(script_context->extension());

  std::string path = gin::V8ToString(arguments[0]);

  RequestResult result(RequestResult::HANDLED);
  std::string url = base::StringPrintf(
      "chrome-extension://%s%s%s", script_context->extension()->id().c_str(),
      !path.empty() && path[0] == '/' ? "" : "/", path.c_str());
  result.return_value = gin::StringToV8(script_context->isolate(), url);

  return result;
}

RequestResult RuntimeHooksDelegate::HandleSendMessage(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  DCHECK_EQ(4u, arguments.size());

  std::string target_id;
  if (!GetTarget(script_context, arguments[0], &target_id)) {
    RequestResult result(RequestResult::INVALID_INVOCATION);
    result.error =
        base::StringPrintf(kExtensionIdRequiredErrorTemplate, "sendMessage");
    return result;
  }

  v8::Local<v8::Context> v8_context = script_context->v8_context();
  MessageOptions options;
  if (!arguments[2]->IsNull()) {
    std::string error;
    ParseParamsResult parse_result = ParseMessageOptions(
        v8_context, arguments[2].As<v8::Object>(), false, &options, &error);
    switch (parse_result) {
      case TYPE_ERROR: {
        RequestResult result(RequestResult::INVALID_INVOCATION);
        result.error = std::move(error);
        return result;
      }
      case THROWN:
        return RequestResult(RequestResult::THROWN);
      case SUCCESS:
        break;
    }
  }

  v8::Local<v8::Value> v8_message = arguments[1];
  DCHECK(!v8_message.IsEmpty());
  std::unique_ptr<Message> message =
      messaging_util::MessageFromV8(v8_context, v8_message);
  if (!message) {
    RequestResult result(RequestResult::INVALID_INVOCATION);
    result.error = "Illegal argument to for 'message'.";
    return result;
  }

  v8::Local<v8::Function> response_callback;
  if (!arguments[3]->IsNull())
    response_callback = arguments[3].As<v8::Function>();

  messaging_service_->SendOneTimeMessage(
      script_context, target_id, kSendMessageChannel,
      options.include_tls_channel_id, *message, response_callback);

  return RequestResult(RequestResult::HANDLED);
}

RequestResult RuntimeHooksDelegate::HandleConnect(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  DCHECK_EQ(2u, arguments.size());

  std::string target_id;
  if (!GetTarget(script_context, arguments[0], &target_id)) {
    RequestResult result(RequestResult::INVALID_INVOCATION);
    result.error =
        base::StringPrintf(kExtensionIdRequiredErrorTemplate, "connect");
    return result;
  }

  MessageOptions options;
  if (!arguments[1]->IsNull()) {
    std::string error;
    ParseParamsResult parse_result = ParseMessageOptions(
        script_context->v8_context(), arguments[1].As<v8::Object>(), true,
        &options, &error);
    switch (parse_result) {
      case TYPE_ERROR: {
        RequestResult result(RequestResult::INVALID_INVOCATION);
        result.error = std::move(error);
        return result;
      }
      case THROWN:
        return RequestResult(RequestResult::THROWN);
      case SUCCESS:
        break;
    }
  }

  gin::Handle<GinPort> port = messaging_service_->Connect(
      script_context, target_id, options.channel_name,
      options.include_tls_channel_id);
  DCHECK(!port.IsEmpty());

  RequestResult result(RequestResult::HANDLED);
  result.return_value = port.ToV8();
  return result;
}

}  // namespace extensions

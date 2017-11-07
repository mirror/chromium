// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/runtime_hooks_delegate.h"

#include "base/containers/span.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/v8_value_converter.h"
#include "extensions/common/api/messaging/message.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest.h"
#include "extensions/renderer/bindings/api_signature.h"
#include "extensions/renderer/extension_frame_helper.h"
#include "extensions/renderer/message_target.h"
#include "extensions/renderer/messaging_util.h"
#include "extensions/renderer/native_renderer_messaging_service.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/script_context_set.h"
#include "gin/converter.h"

namespace extensions {

namespace {
using RequestResult = APIBindingHooks::RequestResult;

// Massages the sendMessage() arguments into the expected schema. These
// arguments are ambiguous (could match multiple signatures), so we can't just
// rely on the normal signature parsing. Sets |arguments| to the result if
// successful; otherwise leaves |arguments| untouched. (If the massage is
// unsuccessful, our normal argument parsing code should throw a reasonable
// error.
void MassageSendMessageArguments(
    v8::Isolate* isolate,
    std::vector<v8::Local<v8::Value>>* arguments_out) {
  LOG(WARNING) << "massing arguments";
  base::span<const v8::Local<v8::Value>> arguments = *arguments_out;
  if (arguments.empty() || arguments.size() > 4u)
    return;

  LOG(WARNING) << "Size: " << arguments.size();
  v8::Local<v8::Value> target_id = v8::Null(isolate);
  v8::Local<v8::Value> message = v8::Null(isolate);
  v8::Local<v8::Value> options = v8::Null(isolate);
  v8::Local<v8::Value> response_callback = v8::Null(isolate);

  // If the last argument is a function, it is the response callback.
  // Ignore it for the purposes of further argument parsing.
  if ((*arguments.rbegin())->IsFunction()) {
    LOG(WARNING) << "popped function";
    response_callback = *arguments.rbegin();
    arguments = arguments.first(arguments.size() - 1);
  }

  switch (arguments.size()) {
    case 0:
      LOG(WARNING) << "No message";
      // Required argument (message) is missing.
      // Early-out and rely on normal signature parsing to report this error.
      return;
    case 1:
      LOG(WARNING) << "Message is 0th";
      // Argument must be the message.
      message = arguments[0];
      break;
    case 2:
      // Assume the meaning is (id, message) if id would be a string.
      // Otherwise the meaning is (message, options).
      if (arguments[0]->IsString()) {
        LOG(WARNING) << "Id 0, message 1";
        target_id = arguments[0];
        message = arguments[1];
      } else {
        LOG(WARNING) << "message 0, options 1";
        message = arguments[0];
        options = arguments[1];
      }
      break;
    case 3:
      LOG(WARNING) << "one of each";
      // The meaning in this case is unambiguous.
      target_id = arguments[0];
      message = arguments[1];
      options = arguments[2];
      break;
    case 4:
      // Too many arguments. Early-out and rely on normal signature parsing to
      // report this error.
      return;
    default:
      NOTREACHED();
  }

  LOG(WARNING) << "Parsed";
  *arguments_out = {target_id, message, options, response_callback};
}

// Handler for the extensionId property on chrome.runtime.
void GetExtensionId(v8::Local<v8::Name> property_name,
                    const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = info.Holder()->CreationContext();

  ScriptContext* script_context =
      ScriptContextSet::GetContextByV8Context(context);
  // This could potentially be invoked after the script context is removed
  // (unlike the handler calls, which should only be invoked for valid
  // contexts).
  if (script_context && script_context->extension()) {
    info.GetReturnValue().Set(
        gin::StringToSymbol(isolate, script_context->extension()->id()));
  }
}

constexpr char kGetManifest[] = "runtime.getManifest";
constexpr char kGetURL[] = "runtime.getURL";
constexpr char kConnect[] = "runtime.connect";
constexpr char kConnectNative[] = "runtime.connectNative";
constexpr char kSendMessage[] = "runtime.sendMessage";
constexpr char kSendNativeMessage[] = "runtime.sendNativeMessage";
constexpr char kGetBackgroundPage[] = "runtime.getBackgroundPage";
constexpr char kGetPackageDirectoryEntry[] = "runtime.getPackageDirectoryEntry";

void GetBackgroundPageCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  LOG(WARNING) << "Getting background page callback";
  v8::Isolate* isolate = info.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = info.Holder()->CreationContext();

  DCHECK(!info.Data().IsEmpty());
  if (info.Data()->IsNull())
    return;

  LOG(WARNING) << "really, though";
  ScriptContext* script_context =
      ScriptContextSet::GetContextByV8Context(context);
  DCHECK(script_context);
  DCHECK(script_context->extension());

  v8::Local<v8::Value> background_page =
      ExtensionFrameHelper::GetV8BackgroundPageMainFrame(
          isolate, script_context->extension()->id());
  v8::Local<v8::Value> args[] = {background_page};
  script_context->SafeCallFunction(
      info.Data().As<v8::Function>(), arraysize(args), args);
}

}  // namespace

RuntimeHooksDelegate::RuntimeHooksDelegate(
    NativeRendererMessagingService* messaging_service,
    const binding::RunJSFunctionSync& run_js_sync)
    : messaging_service_(messaging_service), run_js_sync_(run_js_sync) {}
RuntimeHooksDelegate::~RuntimeHooksDelegate() {}

RequestResult RuntimeHooksDelegate::HandleRequest(
    const std::string& method_name,
    const APISignature* signature,
    v8::Local<v8::Context> context,
    std::vector<v8::Local<v8::Value>>* arguments,
    const APITypeReferenceMap& refs) {
  using Handler = RequestResult (RuntimeHooksDelegate::*)(
      ScriptContext*, const std::vector<v8::Local<v8::Value>>&);
  static const struct {
    Handler handler;
    base::StringPiece method;
  } kHandlers[] = {
      {&RuntimeHooksDelegate::HandleSendMessage, kSendMessage},
      {&RuntimeHooksDelegate::HandleConnect, kConnect},
      {&RuntimeHooksDelegate::HandleGetURL, kGetURL},
      {&RuntimeHooksDelegate::HandleGetManifest, kGetManifest},
      {&RuntimeHooksDelegate::HandleConnectNative, kConnectNative},
      {&RuntimeHooksDelegate::HandleSendNativeMessage, kSendNativeMessage},
      {&RuntimeHooksDelegate::HandleGetBackgroundPage, kGetBackgroundPage},
      {&RuntimeHooksDelegate::HandleGetPackageDirectoryEntryCallback,
       kGetPackageDirectoryEntry},
  };

  ScriptContext* script_context =
      ScriptContextSet::GetContextByV8Context(context);
  DCHECK(script_context);

  Handler handler = nullptr;
  for (const auto& handler_entry : kHandlers) {
    if (handler_entry.method == method_name) {
      handler = handler_entry.handler;
      break;
    }
  }

  if (!handler)
    return RequestResult(RequestResult::NOT_HANDLED);

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

  return (this->*handler)(script_context, parsed_arguments);
}

void RuntimeHooksDelegate::InitializeTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> object_template,
    const APITypeReferenceMap& type_refs) {
  object_template->SetAccessor(gin::StringToSymbol(isolate, "id"),
                               &GetExtensionId);
}

RequestResult RuntimeHooksDelegate::HandleGetManifest(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& parsed_arguments) {
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
  std::string error;
  if (!messaging_util::GetTargetExtensionId(script_context, arguments[0],
                                            "runtime.sendMessage", &target_id,
                                            &error)) {
    RequestResult result(RequestResult::INVALID_INVOCATION);
    result.error = std::move(error);
    return result;
  }

  v8::Local<v8::Context> v8_context = script_context->v8_context();
  messaging_util::MessageOptions options;
  if (!arguments[2]->IsNull()) {
    std::string error;
    messaging_util::ParseOptionsResult parse_result =
        messaging_util::ParseMessageOptions(
            v8_context, arguments[2].As<v8::Object>(),
            messaging_util::PARSE_INCLUDE_TLS_CHANNEL_ID, &options, &error);
    switch (parse_result) {
      case messaging_util::TYPE_ERROR: {
        RequestResult result(RequestResult::INVALID_INVOCATION);
        result.error = std::move(error);
        return result;
      }
      case messaging_util::THROWN:
        return RequestResult(RequestResult::THROWN);
      case messaging_util::SUCCESS:
        break;
    }
  }

  v8::Local<v8::Value> v8_message = arguments[1];
  std::unique_ptr<Message> message =
      messaging_util::MessageFromV8(v8_context, v8_message);
  if (!message) {
    RequestResult result(RequestResult::INVALID_INVOCATION);
    result.error = "Illegal argument to runtime.sendMessage for 'message'.";
    return result;
  }

  v8::Local<v8::Function> response_callback;
  if (!arguments[3]->IsNull())
    response_callback = arguments[3].As<v8::Function>();

  messaging_service_->SendOneTimeMessage(
      script_context, MessageTarget::ForExtension(target_id),
      messaging_util::kSendMessageChannel, options.include_tls_channel_id,
      *message, response_callback);

  return RequestResult(RequestResult::HANDLED);
}

RequestResult RuntimeHooksDelegate::HandleSendNativeMessage(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  DCHECK_EQ(3u, arguments.size());

  std::string application_name = gin::V8ToString(arguments[0]);

  v8::Local<v8::Value> v8_message = arguments[1];
  DCHECK(!v8_message.IsEmpty());
  std::unique_ptr<Message> message =
      messaging_util::MessageFromV8(script_context->v8_context(), v8_message);
  if (!message) {
    RequestResult result(RequestResult::INVALID_INVOCATION);
    result.error =
        "Illegal argument to runtime.sendNativeMessage for 'message'.";
    return result;
  }

  v8::Local<v8::Function> response_callback;
  if (!arguments[2]->IsNull())
    response_callback = arguments[2].As<v8::Function>();

  messaging_service_->SendOneTimeMessage(
      script_context, MessageTarget::ForNativeApp(application_name),
      std::string(), false, *message, response_callback);

  return RequestResult(RequestResult::HANDLED);
}

RequestResult RuntimeHooksDelegate::HandleConnect(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  DCHECK_EQ(2u, arguments.size());

  std::string target_id;
  std::string error;
  if (!messaging_util::GetTargetExtensionId(script_context, arguments[0],
                                            "runtime.connect", &target_id,
                                            &error)) {
    RequestResult result(RequestResult::INVALID_INVOCATION);
    result.error = std::move(error);
    return result;
  }

  messaging_util::MessageOptions options;
  if (!arguments[1]->IsNull()) {
    std::string error;
    messaging_util::ParseOptionsResult parse_result =
        messaging_util::ParseMessageOptions(
            script_context->v8_context(), arguments[1].As<v8::Object>(),
            messaging_util::PARSE_INCLUDE_TLS_CHANNEL_ID |
                messaging_util::PARSE_CHANNEL_NAME,
            &options, &error);
    switch (parse_result) {
      case messaging_util::TYPE_ERROR: {
        RequestResult result(RequestResult::INVALID_INVOCATION);
        result.error = std::move(error);
        return result;
      }
      case messaging_util::THROWN:
        return RequestResult(RequestResult::THROWN);
      case messaging_util::SUCCESS:
        break;
    }
  }

  gin::Handle<GinPort> port = messaging_service_->Connect(
      script_context, MessageTarget::ForExtension(target_id),
      options.channel_name, options.include_tls_channel_id);
  DCHECK(!port.IsEmpty());

  RequestResult result(RequestResult::HANDLED);
  result.return_value = port.ToV8();
  return result;
}

RequestResult RuntimeHooksDelegate::HandleConnectNative(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  DCHECK_EQ(1u, arguments.size());
  DCHECK(arguments[0]->IsString());

  std::string application_name = gin::V8ToString(arguments[0]);
  gin::Handle<GinPort> port = messaging_service_->Connect(
      script_context, MessageTarget::ForNativeApp(application_name),
      std::string(), false);

  RequestResult result(RequestResult::HANDLED);
  result.return_value = port.ToV8();
  return result;
}

RequestResult RuntimeHooksDelegate::HandleGetBackgroundPage(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  DCHECK(script_context->extension());

  RequestResult result(RequestResult::NOT_HANDLED);
  LOG(WARNING) << "Handling";
  if (!v8::Function::New(script_context->v8_context(),
                         &GetBackgroundPageCallback,
                         arguments[0]).ToLocal(&result.custom_callback)) {
    LOG(WARNING) << "Thrown?";
    return RequestResult(RequestResult::THROWN);
  }
  LOG(WARNING) << "Is Empty: " << result.custom_callback.IsEmpty();

  return result;
}

RequestResult RuntimeHooksDelegate::HandleGetPackageDirectoryEntryCallback(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  v8::Isolate* isolate = script_context->isolate();
  v8::Local<v8::Context> v8_context = script_context->v8_context();

  ModuleSystem::NativesEnabledScope enable_natives(
      script_context->module_system());

  v8::Local<v8::Object> file_entry_binding_util;
  if (!script_context->module_system()
           ->Require("fileEntryBindingUtil")
           .ToLocal(&file_entry_binding_util)) {
    NOTREACHED();
    // Abort, and consider the request handled.
    return RequestResult(RequestResult::HANDLED);
  }

  v8::Local<v8::Value> get_bind_directory_entry_callback;
  if (!file_entry_binding_util
           ->Get(v8_context,
                 gin::StringToSymbol(isolate, "getBindDirectoryEntryCallback"))
           .ToLocal(&get_bind_directory_entry_callback) ||
      !get_bind_directory_entry_callback->IsFunction()) {
    NOTREACHED();
    // Abort, and consider the request handled.
    return RequestResult(RequestResult::HANDLED);
  }

  v8::Global<v8::Value> script_result =
      run_js_sync_.Run(get_bind_directory_entry_callback.As<v8::Function>(),
                       v8_context, 0, nullptr);
  v8::Local<v8::Value> callback;
  if (script_result.IsEmpty() ||
      !(callback = script_result.Get(isolate))->IsFunction()) {
    NOTREACHED();
    // Abort, and consider the request handled.
    return RequestResult(RequestResult::HANDLED);
  }

  RequestResult result(RequestResult::NOT_HANDLED);
  result.custom_callback = callback.As<v8::Function>();
  return result;
}

}  // namespace extensions

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/extensions/extension_hooks_delegate.h"

#include "base/strings/stringprintf.h"
#include "content/public/renderer/v8_value_converter.h"
#include "extensions/common/api/messaging/message.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest.h"
#include "extensions/common/view_type.h"
#include "extensions/renderer/bindings/api_signature.h"
#include "extensions/renderer/extension_frame_helper.h"
#include "extensions/renderer/message_target.h"
#include "extensions/renderer/messaging_util.h"
#include "extensions/renderer/native_renderer_messaging_service.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/script_context_set.h"
#include "gin/converter.h"
#include "gin/dictionary.h"

namespace extensions {

namespace {

using RequestResult = APIBindingHooks::RequestResult;

constexpr char kSendRequest[] = "extension.sendRequest";
constexpr char kGetURL[] = "extension.getURL";
constexpr char kGetBackgroundPage[] = "extension.getBackgroundPage";
constexpr char kGetViews[] = "extension.getViews";
constexpr char kGetExtensionTabs[] = "extension.getExtensionTabs";

// We alias a bunch of chrome.extension APIs to their chrome.runtime
// counterparts.
// NOTE(devlin): This is a very simple alias, in which we just return the
// runtime version from the chrome.runtime object. This is important to note
// for a few reasons:
// - Modifications to the chrome.runtime object will affect the return result
//   here. i.e., if script does chrome.runtime.sendMessage = 'some string',
//   then chrome.extension.sendMessage will also be 'some string'.
// - Events will share listeners. i.e., a listener added to
//   chrome.extension.onMessage will fire from a runtime.onMessage event, and
//   vice versa.
// All of these APIs have been deprecated, and are no longer even documented,
// but still have usage. This is the cheap workaround that JS bindings have been
// using, and, while not robust, it should be secure, so use it native bindings,
// too.
void GetAliasedFeature(v8::Local<v8::Name> property_name,
                       const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = info.Holder()->CreationContext();

  v8::TryCatch try_catch(isolate);
  v8::Local<v8::Value> chrome;
  if (!context->Global()
           ->Get(context, gin::StringToSymbol(isolate, "chrome"))
           .ToLocal(&chrome) ||
      !chrome->IsObject()) {
    return;
  }

  v8::Local<v8::Value> runtime;
  if (!chrome.As<v8::Object>()
           ->Get(context, gin::StringToSymbol(isolate, "runtime"))
           .ToLocal(&runtime) ||
      !runtime->IsObject()) {
    return;
  }

  v8::Local<v8::Object> runtime_obj = runtime.As<v8::Object>();
  v8::Maybe<bool> has_property =
      runtime_obj->HasRealNamedProperty(context, property_name);
  if (!has_property.IsJust() || !has_property.FromJust())
    return;

  info.GetReturnValue().Set(
      runtime_obj->Get(context, property_name).ToLocalChecked());
}

}  // namespace

ExtensionHooksDelegate::ExtensionHooksDelegate(
    NativeRendererMessagingService* messaging_service)
    : messaging_service_(messaging_service) {}
ExtensionHooksDelegate::~ExtensionHooksDelegate() {}

RequestResult ExtensionHooksDelegate::HandleRequest(
    const std::string& method_name,
    const APISignature* signature,
    v8::Local<v8::Context> context,
    std::vector<v8::Local<v8::Value>>* arguments,
    const APITypeReferenceMap& refs) {
  // TODO(devlin): This logic is the same in the RuntimeCustomHooksDelegate -
  // would it make sense to share it?
  using Handler = RequestResult (ExtensionHooksDelegate::*)(
      ScriptContext*, const std::vector<v8::Local<v8::Value>>&);
  static struct {
    Handler handler;
    base::StringPiece method;
  } kHandlers[] = {
      {&ExtensionHooksDelegate::HandleSendRequest, kSendRequest},
      {&ExtensionHooksDelegate::HandleGetURL, kGetURL},
      {&ExtensionHooksDelegate::HandleGetBackgroundPage, kGetBackgroundPage},
      {&ExtensionHooksDelegate::HandleGetExtensionTabs, kGetExtensionTabs},
      {&ExtensionHooksDelegate::HandleGetViews, kGetViews},
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

void ExtensionHooksDelegate::InitializeTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> object_template,
    const APITypeReferenceMap& type_refs) {
  static const char* const kAliases[] = {
      "connect",   "connectNative",     "sendMessage", "sendNativeMessage",
      "onConnect", "onConnectExternal", "onMessage",   "onMessageExternal",
  };

  for (const auto* alias : kAliases) {
    object_template->SetAccessor(gin::StringToSymbol(isolate, alias),
                                 &GetAliasedFeature);
  }
}

RequestResult ExtensionHooksDelegate::HandleSendRequest(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  DCHECK_EQ(3u, arguments.size());

  std::string target_id;
  std::string error;
  if (!messaging_util::GetTargetExtensionId(script_context, arguments[0],
                                            "extension.sendRequest", &target_id,
                                            &error)) {
    RequestResult result(RequestResult::INVALID_INVOCATION);
    result.error = std::move(error);
    return result;
  }

  v8::Local<v8::Value> v8_message = arguments[1];
  std::unique_ptr<Message> message =
      messaging_util::MessageFromV8(script_context->v8_context(), v8_message);
  if (!message) {
    RequestResult result(RequestResult::INVALID_INVOCATION);
    result.error = "Illegal argument to extension.sendRequest for 'message'.";
    return result;
  }

  v8::Local<v8::Function> response_callback;
  if (!arguments[2]->IsNull())
    response_callback = arguments[2].As<v8::Function>();

  messaging_service_->SendOneTimeMessage(
      script_context, MessageTarget::ForExtension(target_id),
      messaging_util::kSendRequestChannel, false, *message, response_callback);

  return RequestResult(RequestResult::HANDLED);
}

RequestResult ExtensionHooksDelegate::HandleGetURL(
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

APIBindingHooks::RequestResult ExtensionHooksDelegate::HandleGetViews(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  const Extension* extension = script_context->extension();
  DCHECK(extension);

  ViewType view_type = VIEW_TYPE_INVALID;
  int window_id = extension_misc::kUnknownWindowId;
  int tab_id = extension_misc::kUnknownTabId;

  if (!arguments[0]->IsNull()) {
    gin::Dictionary options_dict(script_context->isolate(),
                                 arguments[0].As<v8::Object>());
    v8::Local<v8::Value> v8_window_id;
    v8::Local<v8::Value> v8_tab_id;
    v8::Local<v8::Value> v8_view_type;
    if (!options_dict.Get("windowId", &v8_window_id) ||
        !options_dict.Get("tabId", &v8_tab_id) ||
        !options_dict.Get("type", &v8_view_type)) {
      return RequestResult(RequestResult::THROWN);
    }

    if (!v8_window_id->IsUndefined() && !v8_window_id->IsNull()) {
      if (!v8_window_id->IsInt32())
        return RequestResult("properties.windowId must be an integer.");
      window_id = v8_window_id->Int32Value();
    }

    if (!v8_tab_id->IsUndefined() && !v8_tab_id->IsNull()) {
      if (!v8_tab_id->IsInt32())
        return RequestResult("properties.tabId must be an integer.");
      tab_id = v8_tab_id->Int32Value();
    }

    if (!v8_view_type->IsUndefined() && !v8_view_type->IsNull()) {
      bool success = false;
      if (v8_view_type->IsString()) {
        std::string view_type_string =
            base::ToUpperASCII(gin::V8ToString(v8_view_type));
        if (view_type_string == "ALL" ||
            GetViewTypeFromString(view_type_string, &view_type)) {
          success = true;
        }
      }
      if (!success)
        return RequestResult("Invalid value for properties.type.");
    }
  }

  RequestResult result(RequestResult::HANDLED);
  result.return_value =
      ExtensionFrameHelper::GetV8MainFrames(script_context->v8_context(),
                                            extension->id(), window_id, tab_id,
                                            view_type);
  return result;
}

RequestResult ExtensionHooksDelegate::HandleGetExtensionTabs(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  const Extension* extension = script_context->extension();
  DCHECK(extension);

  ViewType view_type = VIEW_TYPE_TAB_CONTENTS;
  int window_id = extension_misc::kUnknownWindowId;
  int tab_id = extension_misc::kUnknownTabId;

  if (!arguments[0]->IsNull())
    tab_id = arguments[0]->Int32Value();

  RequestResult result(RequestResult::HANDLED);
  result.return_value =
      ExtensionFrameHelper::GetV8MainFrames(script_context->v8_context(),
                                            extension->id(), window_id, tab_id,
                                            view_type);
  return result;
}

RequestResult ExtensionHooksDelegate::HandleGetBackgroundPage(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  const Extension* extension = script_context->extension();
  DCHECK(extension);

  RequestResult result(RequestResult::HANDLED);
  result.return_value =
      ExtensionFrameHelper::GetV8BackgroundPageMainFrame(
          script_context->isolate(), extension->id());
  return result;
}

}  // namespace extensions

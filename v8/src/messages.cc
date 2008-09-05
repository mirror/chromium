
// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#include "v8.h"

#include "api.h"
#include "execution.h"
#include "spaces-inl.h"
#include "top.h"

namespace v8 { namespace internal {


// If no message listeners have been registered this one is called
// by default.
void MessageHandler::DefaultMessageReport(const MessageLocation* loc,
                                          Handle<Object> message_obj) {
  SmartPointer<char> str = GetLocalizedMessage(message_obj);
  if (loc == NULL) {
    PrintF("%s\n", *str);
  } else {
    HandleScope scope;
    Handle<Object> data(loc->script()->name());
    SmartPointer<char> data_str = NULL;
    if (data->IsString())
      data_str = Handle<String>::cast(data)->ToCString(DISALLOW_NULLS);
    PrintF("%s:%i: %s\n", *data_str ? *data_str : "<unknown>",
           loc->start_pos(), *str);
  }
}


void MessageHandler::ReportMessage(const char* msg) {
  PrintF("%s\n", msg);
}


void MessageHandler::ReportMessage(const char* type, MessageLocation* loc,
                                   Vector< Handle<Object> > args) {
  // Build error message object
  HandleScope scope;
  Handle<Object> type_str = Factory::LookupAsciiSymbol(type);
  Handle<Object> array = Factory::NewJSArray(args.length());
  for (int i = 0; i < args.length(); i++)
    SetElement(Handle<JSArray>::cast(array), i, args[i]);

  Handle<JSFunction> fun(Top::global_context()->make_message_fun());
  int start, end;
  Handle<Object> script;
  if (loc) {
    start = loc->start_pos();
    end = loc->end_pos();
    script = GetScriptWrapper(loc->script());
  } else {
    start = end = 0;
    script = Factory::undefined_value();
  }
  Handle<Object> start_handle(Smi::FromInt(start));
  Handle<Object> end_handle(Smi::FromInt(end));
  const int argc = 5;
  Object** argv[argc] = { type_str.location(),
                          array.location(),
                          start_handle.location(),
                          end_handle.location(),
                          script.location() };

  bool caught_exception = false;
  Handle<Object> message =
      Execution::TryCall(fun, Factory::undefined_value(), argc, argv,
                         &caught_exception);
  // If creating the message (in JS code) resulted in an exception, we
  // skip doing the callback. This usually only happens in case of
  // stack overflow exceptions being thrown by the parser when the
  // stack is almost full.
  if (caught_exception) return;

  v8::Local<v8::Message> api_message_obj = v8::Utils::MessageToLocal(message);

  v8::NeanderArray global_listeners(Factory::message_listeners());
  int global_length = global_listeners.length();
  if (global_length == 0) {
    DefaultMessageReport(loc, message);
  } else {
    for (int i = 0; i < global_length; i++) {
      HandleScope scope;
      if (global_listeners.get(i)->IsUndefined()) continue;
      v8::NeanderObject listener(JSObject::cast(global_listeners.get(i)));
      Handle<Proxy> callback_obj(Proxy::cast(listener.get(0)));
      v8::MessageCallback callback =
          FUNCTION_CAST<v8::MessageCallback>(callback_obj->proxy());
      Handle<Object> callback_data(listener.get(1));
      callback(api_message_obj, v8::Utils::ToLocal(callback_data));
    }
  }
}


Handle<String> MessageHandler::GetMessage(Handle<Object> data) {
  Handle<String> fmt_str = Factory::LookupAsciiSymbol("FormatMessage");
  Handle<JSFunction> fun =
      Handle<JSFunction>(
          JSFunction::cast(
              Top::security_context_builtins()->GetProperty(*fmt_str)));
  Object** argv[1] = { data.location() };

  bool caught_exception;
  Handle<Object> result =
      Execution::TryCall(fun, Top::security_context_builtins(), 1, argv,
                         &caught_exception);

  if (caught_exception || !result->IsString()) {
    return Factory::LookupAsciiSymbol("<error>");
  }
  Handle<String> result_string = Handle<String>::cast(result);
  // A string that has been obtained from JS code in this way is
  // likely to be a complicated ConsString of some sort.  We flatten it
  // here to improve the efficiency of converting it to a C string and
  // other operations that are likely to take place (see GetLocalizedMessage
  // for example).
  FlattenString(result_string);
  return result_string;
}


SmartPointer<char> MessageHandler::GetLocalizedMessage(Handle<Object> data) {
  HandleScope scope;
  return GetMessage(data)->ToCString(DISALLOW_NULLS);
}


} }  // namespace v8::internal

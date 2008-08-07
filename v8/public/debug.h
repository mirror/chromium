// Copyright 2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef _V8_DEBUG
#define _V8_DEBUG

#include "v8.h"

#if defined(__GNUC__) && (__GNUC__ >= 4)
#define EXPORT __attribute__((visibility("default")))
#else
#define EXPORT
#endif

/**
 * Debugger support for the V8 JavaScript engine.
 */
namespace v8 {

// Debug events which can occour in the V8 JavaScript engine.
enum DebugEvent {
  Break = 1,
  Exception = 2,
  NewFunction = 3,
  BeforeCompile = 4,
  AfterCompile  = 5,
  PendingRequestProcessed = 6
};


/**
 * Debug event callback function.
 *
 * \param event the debug event from which occoured (from the DebugEvent
 *              enumeration)
 * \param exec_state execution state (JavaScript object)
 * \param event_data event specific data (JavaScript object)
 * \param data value passed by the user to AddDebugEventListener
 */
typedef void (*DebugEventCallback)(DebugEvent event,
                                   Handle<Object> exec_state,
                                   Handle<Object> event_data,
                                   Handle<Value> data);


/**
 * Debug message callback function.
 *
 * \param message the debug message
 * \param length length of the message
 */
typedef void (*DebugMessageHandler)(const uint16_t* message, int length,
                                    void* data);


class EXPORT Debug {
 public:
  // Add a C debug event listener.
  static bool AddDebugEventListener(DebugEventCallback that,
                                    Handle<Value> data = Handle<Value>());

  // Add a JavaScript debug event listener.
  static bool AddDebugEventListener(v8::Handle<v8::Function> that,
                                    Handle<Value> data = Handle<Value>());

  // Remove a C debug event listener.
  static void RemoveDebugEventListener(DebugEventCallback that);

  // Remove a JavaScript debug event listener.
  static void RemoveDebugEventListener(v8::Handle<v8::Function> that);

  // Generate a stack dump.
  static void StackDump();

  // Break execution of JavaScript.
  static void DebugBreak();

  // Message based interface. The message protocol is JSON.
  static void SetMessageHandler(DebugMessageHandler handler, void* data = NULL);
  static void SendCommand(const uint16_t* command, int length);
};


}  // namespace v8


#undef EXPORT


#endif  // _V8_DEBUG

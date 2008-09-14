// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

// The infrastructure used for (localized) message reporting in V8.
//
// Note: there's a big unresolved issue about ownership of the data
// structures used by this framework.

#ifndef V8_MESSAGES_H_
#define V8_MESSAGES_H_

#include "handles-inl.h"

// Forward declaration of MessageLocation.
namespace v8 { namespace internal {
class MessageLocation;
} }  // namespace v8::internal


class V8Message {
 public:
  V8Message(char* type,
            v8::internal::Handle<v8::internal::JSArray> args,
            const v8::internal::MessageLocation* loc) :
      type_(type), args_(args), loc_(loc) { }
  char* type() const { return type_; }
  v8::internal::Handle<v8::internal::JSArray> args() const { return args_; }
  const v8::internal::MessageLocation* loc() const { return loc_; }
 private:
  char* type_;
  v8::internal::Handle<v8::internal::JSArray> const args_;
  const v8::internal::MessageLocation* loc_;
};


namespace v8 { namespace internal {

struct Language;
class SourceInfo;

class MessageLocation {
 public:
  MessageLocation(Handle<Script> script,
                  int start_pos,
                  int end_pos)
      : script_(script),
        start_pos_(start_pos),
        end_pos_(end_pos) { }

  Handle<Script> script() const { return script_; }
  int start_pos() const { return start_pos_; }
  int end_pos() const { return end_pos_; }

 private:
  Handle<Script> script_;
  int start_pos_;
  int end_pos_;
};


// A message handler is a convenience interface for accessing the list
// of message listeners registered in an environment
class MessageHandler {
 public:
  // Report a message (w/o JS heap allocation).
  static void ReportMessage(const char* msg);

  // Report a formatted message (needs JS allocation).
  static void ReportMessage(const char* type,
                            MessageLocation* loc,
                            Vector< Handle<Object> > args);

  static void DefaultMessageReport(const MessageLocation* loc,
                                   Handle<Object> message_obj);
  static Handle<String> GetMessage(Handle<Object> data);
  static SmartPointer<char> GetLocalizedMessage(Handle<Object> data);
};

} }  // namespace v8::internal

#endif  // V8_MESSAGES_H_

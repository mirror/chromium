// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_JSREGEXP_H_
#define V8_JSREGEXP_H_

namespace v8 { namespace internal {

class RegExpImpl {
 public:
  // Creates a regular expression literal in the old space.
  // This function calls the garbage collector if necessary.
  static Handle<Object> CreateRegExpLiteral(Handle<String> pattern,
                                            Handle<String> flags,
                                            bool* has_pending_exception);

  // Returns a string representation of a regular expression.
  // Implements RegExp.prototype.toString, see ECMA-262 section 15.10.6.4.
  // This function calls the garbage collector if necessary.
  static Handle<String> ToString(Handle<Object> value);

  static Handle<Object> JsreCompile(Handle<JSValue> re,
                                    Handle<String> pattern,
                                    Handle<String> flags);

  // Implements RegExp.prototype.exec(string) function.
  // See ECMA-262 section 15.10.6.2.
  // This function calls the garbage collector if necessary.
  static Handle<Object> JsreExec(Handle<JSValue> regexp,
                                 Handle<String> subject,
                                 Handle<Object> index);

  // Call RegExp.prototyp.exec(string) in a loop.
  // Used by String.prototype.match and String.prototype.replace.
  // This function calls the garbage collector if necessary.
  static Handle<Object> JsreExecGlobal(Handle<JSValue> regexp,
                                       Handle<String> subject);

  static void NewSpaceCollectionPrologue();
  static void OldSpaceCollectionPrologue();

 private:
  // Converts a source string to a 16 bit flat string.  The string
  // will be either sequential or it will be a SlicedString backed
  // by a flat string.
  static Handle<String> StringToTwoByte(Handle<String> pattern);
  static Handle<String> CachedStringToTwoByte(Handle<String> pattern);

  static String* last_ascii_string_;
  static String* two_byte_cached_string_;

  // Returns the caputure from the re.
  static int JsreCapture(Handle<JSValue> re);
  static ByteArray* JsreInternal(Handle<JSValue> re);

  // Call jsRegExpExecute once
  static Handle<Object> JsreExecOnce(Handle<JSValue> regexp,
                                     int num_captures,
                                     Handle<String> subject,
                                     int previous_index,
                                     const uc16* utf8_subject,
                                     int* ovector,
                                     int ovector_length);

  // Set the subject cache.  The previous string buffer is not deleted, so the
  // caller should ensure that it doesn't leak.
  static void SetSubjectCache(String* subject, char* utf8_subject,
                              int uft8_length, int character_position,
                              int utf8_position);

  // A one element cache of the last utf8_subject string and its length.  The
  // subject JS String object is cached in the heap.  We also cache a
  // translation between position and utf8 position.
  static char* utf8_subject_cache_;
  static int utf8_length_cache_;
  static int utf8_position_;
  static int character_position_;
};


} }  // namespace v8::internal

#endif  // V8_JSREGEXP_H_

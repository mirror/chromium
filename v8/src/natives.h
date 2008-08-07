// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_NATIVES_H_
#define V8_NATIVES_H_

namespace v8 { namespace internal {

typedef bool (*NativeSourceCallback)(Vector<const char> name,
                                     Vector<const char> source,
                                     int index);

class Natives {
 public:
  // Number of built-in scripts.
  static int GetBuiltinsCount();
  // Number of delayed/lazy loading scripts.
  static int GetDelayCount();

  // These are used to access built-in scripts.
  // The delayed script has an index in the interval [0, GetDelayCount()).
  // The non-delayed script has an index in the interval
  // [GetDelayCount(), GetNativesCount()).
  static int GetIndex(const char* name);
  static Vector<const char> GetScriptSource(int index);
  static Vector<const char> GetScriptName(int index);
};

} }  // namespace v8::internal

#endif  // V8_NATIVES_H_

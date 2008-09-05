// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_ARGUMENTS_H_
#define V8_ARGUMENTS_H_

namespace v8 { namespace internal {

// Arguments provides access to runtime call parameters.
//
// It uses the fact that the instance fields of Arguments
// (length_, arguments_) are "overlayed" with the parameters
// (no. of parameters, and the parameter pointer) passed so
// that inside the C++ function, the parameters passed can
// be accessed conveniently:
//
//   Object* Runtime_function(Arguments args) {
//     ... use args[i] here ...
//   }

class Arguments BASE_EMBEDDED {
 public:
  Object*& operator[] (int index) {
    ASSERT(0 <= index && index <= length_);
    return arguments_[-index];
  }

  template <class S> Handle<S> at(int index) {
    Object** value = &((*this)[index]);
    // This cast checks that the object we're accessing does indeed have the
    // expected type.
    S::cast(*value);
    return Handle<S>(reinterpret_cast<S**>(value));
  }

  // Get the total number of arguments including the receiver.
  int length() const { return length_ + 1; }

 private:
  int length_;
  Object** arguments_;
};

} }  // namespace v8::internal

#endif  // V8_ARGUMENTS_H_

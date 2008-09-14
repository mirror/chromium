// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_CODE_H_
#define V8_CODE_H_

namespace v8 { namespace internal {


// Wrapper class for passing expected and actual parameter counts as
// either registers or immediate values. Used to make sure that the
// caller provides exactly the expected number of parameters to the
// callee.
class ParameterCount BASE_EMBEDDED {
 public:
  explicit ParameterCount(Register reg)
      : reg_(reg), immediate_(0) { }
  explicit ParameterCount(int immediate)
      : reg_(no_reg), immediate_(immediate) { }

  bool is_reg() const { return !reg_.is(no_reg); }
  bool is_immediate() const { return !is_reg(); }

  Register reg() const {
    ASSERT(is_reg());
    return reg_;
  }
  int immediate() const {
    ASSERT(is_immediate());
    return immediate_;
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ParameterCount);

  const Register reg_;
  const int immediate_;
};


} }  // namespace v8::internal

#endif  // V8_CODE_H_

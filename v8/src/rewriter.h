// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_REWRITER_H_
#define V8_REWRITER_H_

namespace v8 { namespace internal {


// Currently, the rewriter takes function literals (only top-level)
// and rewrites them to return the value of the last expression in
// them.
//
// The rewriter adds a (hidden) variable, called .result, to the
// activation, and tries to figure out where it needs to store into
// this variable. If the variable is ever used, we conclude by adding
// a return statement that returns the variable to the body of the
// given function.

class Rewriter {
 public:
  static bool Process(FunctionLiteral* function);
};


} }  // namespace v8::internal

#endif  // V8_REWRITER_H_

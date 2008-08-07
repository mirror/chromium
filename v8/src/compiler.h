// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_COMPILER_H_
#define V8_COMPILER_H_

#include "parser.h"

namespace v8 { namespace internal {

// The V8 compiler
//
// General strategy: Source code is translated into an anonymous function w/o
// parameters which then can be executed. If the source code contains other
// functions, they will be compiled and allocated as part of the compilation
// of the source code.

// Please note this interface returns function boilerplates.
// This means you need to call Factory::NewFunctionFromBoilerplate
// before you have a real function with context.

class Compiler : public AllStatic {
 public:
  // All routines return a JSFunction.
  // If an error occurs an exception is raised and
  // the return handle contains NULL.

  // Compile a String source within a context.
  static Handle<JSFunction> Compile(Handle<String> source,
                                    Handle<String> script_name,
                                    int line_offset, int column_offset,
                                    v8::Extension* extension,
                                    ScriptDataImpl* script_Data);

  // Compile a String source within a context for Eval.
  static Handle<JSFunction> CompileEval(bool is_global, Handle<String> source);

  // Compile from function info (used for lazy compilation). Returns
  // true on success and false if the compilation resulted in a stack
  // overflow.
  static bool CompileLazy(Handle<SharedFunctionInfo> shared);
};

} }  // namespace v8::internal

#endif  // V8_COMPILER_H_

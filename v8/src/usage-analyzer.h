// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_USAGE_ANALYSER_H_
#define V8_USAGE_ANALYSER_H_

namespace v8 { namespace internal {

// Compute usage counts for all variables.
// Used for variable allocation.
bool AnalyzeVariableUsage(FunctionLiteral* lit);

} }  // namespace v8::internal

#endif  // V8_USAGE_ANALYSER_H_

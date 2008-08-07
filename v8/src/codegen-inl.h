// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>


#ifndef V8_CODEGEN_INL_H_
#define V8_CODEGEN_INL_H_

#include "codegen.h"


namespace v8 { namespace internal {


// -----------------------------------------------------------------------------
// Support for "structured" code comments.
//
// By selecting matching brackets in disassembler output,
// code segments can be identified more easily.

#ifdef DEBUG

class Comment BASE_EMBEDDED {
 public:
  Comment(MacroAssembler* masm, const char* msg)
    : masm_(masm),
      msg_(msg) {
    masm_->RecordComment(msg);
  }

  ~Comment() {
    if (msg_[0] == '[')
      masm_->RecordComment("]");
  }

 private:
  MacroAssembler* masm_;
  const char* msg_;
};

#else

class Comment BASE_EMBEDDED {
 public:
  Comment(MacroAssembler*, const char*)  {}
};

#endif  // DEBUG


} }  // namespace v8::internal

#endif  // V8_CODEGEN_INL_H_

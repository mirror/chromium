// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#include "v8.h"

#include "frames-inl.h"

namespace v8 { namespace internal {


StackFrame::Type StackFrame::ComputeType(State* state) {
  ASSERT(state->fp != NULL);
  if (StandardFrame::IsArgumentsAdaptorFrame(state->fp)) {
    return ARGUMENTS_ADAPTOR;
  }
  // The marker and function offsets overlap. If the marker isn't a
  // smi then the frame is a JavaScript frame -- and the marker is
  // really the function.
  const int offset = StandardFrameConstants::kMarkerOffset;
  Object* marker = Memory::Object_at(state->fp + offset);
  if (!marker->IsSmi()) return JAVA_SCRIPT;
  return static_cast<StackFrame::Type>(Smi::cast(marker)->value());
}


StackFrame::Type ExitFrame::GetStateForFramePointer(Address fp, State* state) {
  if (fp == 0) return NONE;
  // Compute the stack pointer.
  Address sp = Memory::Address_at(fp + ExitFrameConstants::kSPOffset);
  // Fill in the state.
  state->fp = fp;
  state->sp = sp;
  state->pc_address = reinterpret_cast<Address*>(sp - 1 * kPointerSize);
  // Determine frame type.
  if (Memory::Address_at(fp + ExitFrameConstants::kDebugMarkOffset) != 0) {
    return EXIT_DEBUG;
  } else {
    return EXIT;
  }
}


void ExitFrame::Iterate(ObjectVisitor* v) const {
  // Exit frames on IA-32 do not contain any pointers. The arguments
  // are traversed as part of the expression stack of the calling
  // frame.
}


void ExitFrame::RestoreCalleeSavedRegisters(Object* buffer[]) const {
  // Do nothing.
}


int JavaScriptFrame::GetProvidedParametersCount() const {
  return ComputeParametersCount();
}


Address JavaScriptFrame::GetCallerStackPointer() const {
  int arguments;
  if (Heap::gc_state() != Heap::NOT_IN_GC) {
    // The arguments for cooked frames are traversed as if they were
    // expression stack elements of the calling frame. The reason for
    // this rather strange decision is that we cannot access the
    // function during mark-compact GCs when the stack is cooked.
    // In fact accessing heap objects (like function->shared() below)
    // at all during GC is problematic.
    arguments = 0;
  } else {
    // Compute the number of arguments by getting the number of formal
    // parameters of the function. We must remember to take the
    // receiver into account (+1).
    JSFunction* function = JSFunction::cast(this->function());
    arguments = function->shared()->formal_parameter_count() + 1;
  }
  const int offset = StandardFrameConstants::kCallerSPOffset;
  return fp() + offset + (arguments * kPointerSize);
}


Address ArgumentsAdaptorFrame::GetCallerStackPointer() const {
  const int arguments = Smi::cast(GetExpression(0))->value();
  const int offset = StandardFrameConstants::kCallerSPOffset;
  return fp() + offset + (arguments + 1) * kPointerSize;
}


Address InternalFrame::GetCallerStackPointer() const {
  // Internal frames have no arguments. The stack pointer of the
  // caller is at a fixed offset from the frame pointer.
  return fp() + StandardFrameConstants::kCallerSPOffset;
}


RegList JavaScriptFrame::FindCalleeSavedRegisters() const {
  return 0;
}


void JavaScriptFrame::RestoreCalleeSavedRegisters(Object* buffer[]) const {
  // Do nothing.
}


Code* JavaScriptFrame::FindCode() const {
  JSFunction* function = JSFunction::cast(this->function());
  return function->shared()->code();
}


} }  // namespace v8::internal

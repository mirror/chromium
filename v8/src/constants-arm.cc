// Copyright 2008 Google Inc. All Rights Reserved.
// <<license>>

#include "v8.h"

#include "constants-arm.h"


namespace assembler { namespace arm {

// Instances of class Instr cannot be created. References to Instr can only be
// created by calling Instr::At(pc).
Instr::Instr() {
  UNREACHABLE();
}

} }  // namespace assembler::arm

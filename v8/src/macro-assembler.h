// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_MACRO_ASSEMBLER_H_
#define V8_MACRO_ASSEMBLER_H_

#if defined(ARM) || defined (__arm__) || defined(__thumb__)

#include "constants-arm.h"
#include "assembler.h"
#include "assembler-arm.h"
#include "assembler-arm-inl.h"
#include "code.h"  // must be after assembler_*.h
#include "macro-assembler-arm.h"

#else  // ia32

#include "assembler.h"
#include "assembler-ia32.h"
#include "assembler-ia32-inl.h"
#include "code.h"  // must be after assembler_*.h
#include "macro-assembler-ia32.h"

#endif

#endif  // V8_MACRO_ASSEMBLER_H_

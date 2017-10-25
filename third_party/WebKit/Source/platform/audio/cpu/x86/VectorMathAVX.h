// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VectorMathAVX_h
#define VectorMathAVX_h

#include "build/build_config.h"

#if defined(ARCH_CPU_X86_FAMILY) && !defined(OS_MACOSX)

namespace blink {
namespace VectorMath {
namespace AVX {

constexpr size_t kBitsPerRegister = 256u;
constexpr size_t kPackedFloatsPerRegister = kBitsPerRegister / 32u;
constexpr size_t kFramesToProcessMask = ~(kPackedFloatsPerRegister - 1u);

bool IsAligned(const float*);

void Vadd(const float* source1p,
          const float* source2p,
          float* dest_p,
          size_t frames_to_process);

void Vclip(const float* source_p,
           const float* low_threshold_p,
           const float* high_threshold_p,
           float* dest_p,
           size_t frames_to_process);

void Vmaxmgv(const float* source_p, float* max_p, size_t frames_to_process);

void Vmul(const float* source1p,
          const float* source2p,
          float* dest_p,
          size_t frames_to_process);

void Vsma(const float* source_p,
          const float* scale,
          float* dest_p,
          size_t frames_to_process);

void Vsmul(const float* source_p,
           const float* scale,
           float* dest_p,
           size_t frames_to_process);

void Vsvesq(const float* source_p, float* sum_p, size_t frames_to_process);

void Zvmul(const float* real1p,
           const float* imag1p,
           const float* real2p,
           const float* imag2p,
           float* real_dest_p,
           float* imag_dest_p,
           size_t frames_to_process);

}  // namespace AVX
}  // namespace VectorMath
}  // namespace blink

#endif  // defined(ARCH_CPU_X86_FAMILY) && !defined(OS_MACOSX)

#endif  // VectorMathAVX_h

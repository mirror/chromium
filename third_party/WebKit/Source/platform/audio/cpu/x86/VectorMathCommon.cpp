// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"

#if defined(ARCH_CPU_X86_FAMILY) && !defined(OS_MACOSX)

#include "platform/wtf/Assertions.h"

#include <algorithm>

#if defined(__AVX__)
#include <immintrin.h>
#else
#include <xmmintrin.h>
#endif

namespace blink {
namespace VectorMath {

#if defined(__AVX__)
namespace AVX {
#else
namespace SSE {
#endif

namespace {
#if defined(__AVX__)

#define MM_PS(name) _mm256_##name##_ps

using MType = __m256;

#else

#define MM_PS(name) _mm_##name##_ps

using MType = __m128;

constexpr size_t kBitsPerPack = 128u;
constexpr size_t kFloatsPerPack = kBitsPerPack / 32u;
constexpr size_t kFramesToProcessMask = ~(kFloatsPerPack - 1u);

#endif
}

#if !defined(__AVX__)
namespace {
#endif  // !defined(__AVX__)

bool IsAligned(const float* p) {
  constexpr size_t kBytesPerPack = kBitsPerPack / 8u;
  return !(reinterpret_cast<size_t>(p) & (kBytesPerPack - 1u));
}

// dest[k] = source1[k] + source2[k]
void Vadd(const float* source1p,
          const float* source2p,
          float* dest_p,
          size_t frames_to_process) {
  const float* const source1_end_p = source1p + frames_to_process;

  DCHECK(IsAligned(source1p));
  DCHECK_EQ(0u, frames_to_process % kFloatsPerPack);

  if (IsAligned(source2p)) {
    if (IsAligned(dest_p)) {
      while (source1p < source1_end_p) {
        MType m_source1 = MM_PS(load)(source1p);
        MType m_source2 = MM_PS(load)(source2p);
        MType m_dest = MM_PS(add)(m_source1, m_source2);
        MM_PS(store)(dest_p, m_dest);
        source1p += kFloatsPerPack;
        source2p += kFloatsPerPack;
        dest_p += kFloatsPerPack;
      }
    } else {
      while (source1p < source1_end_p) {
        MType m_source1 = MM_PS(load)(source1p);
        MType m_source2 = MM_PS(load)(source2p);
        MType m_dest = MM_PS(add)(m_source1, m_source2);
        MM_PS(storeu)(dest_p, m_dest);
        source1p += kFloatsPerPack;
        source2p += kFloatsPerPack;
        dest_p += kFloatsPerPack;
      }
    }
  } else {
    if (IsAligned(dest_p)) {
      while (source1p < source1_end_p) {
        MType m_source1 = MM_PS(load)(source1p);
        MType m_source2 = MM_PS(loadu)(source2p);
        MType m_dest = MM_PS(add)(m_source1, m_source2);
        MM_PS(store)(dest_p, m_dest);
        source1p += kFloatsPerPack;
        source2p += kFloatsPerPack;
        dest_p += kFloatsPerPack;
      }
    } else {
      while (source1p < source1_end_p) {
        MType m_source1 = MM_PS(load)(source1p);
        MType m_source2 = MM_PS(loadu)(source2p);
        MType m_dest = MM_PS(add)(m_source1, m_source2);
        MM_PS(storeu)(dest_p, m_dest);
        source1p += kFloatsPerPack;
        source2p += kFloatsPerPack;
        dest_p += kFloatsPerPack;
      }
    }
  }
}

// dest[k] = clip(source[k], low_threshold, high_threshold)
//         = max(low_threshold, min(high_threshold, source[k]))
void Vclip(const float* source_p,
           const float* low_threshold_p,
           const float* high_threshold_p,
           float* dest_p,
           size_t frames_to_process) {
  const float* const source_end_p = source_p + frames_to_process;

  DCHECK(IsAligned(source_p));
  DCHECK_EQ(0u, frames_to_process % kFloatsPerPack);

  MType m_low_threshold = MM_PS(set1)(*low_threshold_p);
  MType m_high_threshold = MM_PS(set1)(*high_threshold_p);

  if (IsAligned(dest_p)) {
    while (source_p < source_end_p) {
      MType m_source = MM_PS(load)(source_p);
      MType m_dest =
          MM_PS(max)(m_low_threshold, MM_PS(min)(m_high_threshold, m_source));
      MM_PS(store)(dest_p, m_dest);
      source_p += kFloatsPerPack;
      dest_p += kFloatsPerPack;
    }
  } else {
    while (source_p < source_end_p) {
      MType m_source = MM_PS(load)(source_p);
      MType m_dest =
          MM_PS(max)(m_low_threshold, MM_PS(min)(m_high_threshold, m_source));
      MM_PS(storeu)(dest_p, m_dest);
      source_p += kFloatsPerPack;
      dest_p += kFloatsPerPack;
    }
  }
}

// max = max(abs(source[k])) for all k
void Vmaxmgv(const float* source_p, float* max_p, size_t frames_to_process) {
  constexpr uint32_t kMask = 0x7FFFFFFFu;

  const float* const source_end_p = source_p + frames_to_process;

  DCHECK(IsAligned(source_p));
  DCHECK_EQ(0u, frames_to_process % kFloatsPerPack);

  MType m_mask = MM_PS(set1)(*reinterpret_cast<const float*>(&kMask));
  MType m_max = MM_PS(setzero)();

  while (source_p < source_end_p) {
    MType m_source = MM_PS(load)(source_p);
    // Calculate the absolute value by ANDing the source with the mask,
    // which will set the sign bit to 0.
    m_source = MM_PS(and)(m_source, m_mask);
    m_max = MM_PS(max)(m_source, m_max);
    source_p += kFloatsPerPack;
  }

  // Combine the packed floats.
  const float* maxes = reinterpret_cast<const float*>(&m_max);
  for (unsigned i = 0u; i < kFloatsPerPack; ++i)
    *max_p = std::max(*max_p, maxes[i]);
}

// dest[k] = source1[k] * source2[k]
void Vmul(const float* source1p,
          const float* source2p,
          float* dest_p,
          size_t frames_to_process) {
  const float* const source1_end_p = source1p + frames_to_process;

  DCHECK(IsAligned(source1p));
  DCHECK_EQ(0u, frames_to_process % kFloatsPerPack);

  if (IsAligned(source2p)) {
    if (IsAligned(dest_p)) {
      while (source1p < source1_end_p) {
        MType m_source1 = MM_PS(load)(source1p);
        MType m_source2 = MM_PS(load)(source2p);
        MType m_dest = MM_PS(mul)(m_source1, m_source2);
        MM_PS(store)(dest_p, m_dest);
        source1p += kFloatsPerPack;
        source2p += kFloatsPerPack;
        dest_p += kFloatsPerPack;
      }
    } else {
      while (source1p < source1_end_p) {
        MType m_source1 = MM_PS(load)(source1p);
        MType m_source2 = MM_PS(load)(source2p);
        MType m_dest = MM_PS(mul)(m_source1, m_source2);
        MM_PS(storeu)(dest_p, m_dest);
        source1p += kFloatsPerPack;
        source2p += kFloatsPerPack;
        dest_p += kFloatsPerPack;
      }
    }
  } else {
    if (IsAligned(dest_p)) {
      while (source1p < source1_end_p) {
        MType m_source1 = MM_PS(load)(source1p);
        MType m_source2 = MM_PS(loadu)(source2p);
        MType m_dest = MM_PS(mul)(m_source1, m_source2);
        MM_PS(store)(dest_p, m_dest);
        source1p += kFloatsPerPack;
        source2p += kFloatsPerPack;
        dest_p += kFloatsPerPack;
      }
    } else {
      while (source1p < source1_end_p) {
        MType m_source1 = MM_PS(load)(source1p);
        MType m_source2 = MM_PS(loadu)(source2p);
        MType m_dest = MM_PS(mul)(m_source1, m_source2);
        MM_PS(storeu)(dest_p, m_dest);
        source1p += kFloatsPerPack;
        source2p += kFloatsPerPack;
        dest_p += kFloatsPerPack;
      }
    }
  }
}

// dest[k] += scale * source[k]
void Vsma(const float* source_p,
          const float* scale,
          float* dest_p,
          size_t frames_to_process) {
  const float* const source_end_p = source_p + frames_to_process;

  DCHECK(IsAligned(source_p));
  DCHECK_EQ(0u, frames_to_process % kFloatsPerPack);

  const MType m_scale = MM_PS(set1)(*scale);

  if (IsAligned(dest_p)) {
    while (source_p < source_end_p) {
      MType m_source = MM_PS(load)(source_p);
      MType m_dest = MM_PS(load)(dest_p);
      m_dest = MM_PS(add)(m_dest, MM_PS(mul)(m_scale, m_source));
      MM_PS(store)(dest_p, m_dest);
      source_p += kFloatsPerPack;
      dest_p += kFloatsPerPack;
    }
  } else {
    while (source_p < source_end_p) {
      MType m_source = MM_PS(load)(source_p);
      MType m_dest = MM_PS(loadu)(dest_p);
      m_dest = MM_PS(add)(m_dest, MM_PS(mul)(m_scale, m_source));
      MM_PS(storeu)(dest_p, m_dest);
      source_p += kFloatsPerPack;
      dest_p += kFloatsPerPack;
    }
  }
}

// dest[k] = scale * source[k]
void Vsmul(const float* source_p,
           const float* scale,
           float* dest_p,
           size_t frames_to_process) {
  const float* const source_end_p = source_p + frames_to_process;

  DCHECK(IsAligned(source_p));
  DCHECK_EQ(0u, frames_to_process % kFloatsPerPack);

  const MType m_scale = MM_PS(set1)(*scale);

  if (IsAligned(dest_p)) {
    while (source_p < source_end_p) {
      MType m_source = MM_PS(load)(source_p);
      MType m_dest = MM_PS(mul)(m_scale, m_source);
      MM_PS(store)(dest_p, m_dest);
      source_p += kFloatsPerPack;
      dest_p += kFloatsPerPack;
    }
  } else {
    while (source_p < source_end_p) {
      MType m_source = MM_PS(load)(source_p);
      MType m_dest = MM_PS(mul)(m_scale, m_source);
      MM_PS(storeu)(dest_p, m_dest);
      source_p += kFloatsPerPack;
      dest_p += kFloatsPerPack;
    }
  }
}

// sum += sum(source[k]^2) for all k
void Vsvesq(const float* source_p, float* sum_p, size_t frames_to_process) {
  const float* const source_end_p = source_p + frames_to_process;

  DCHECK(IsAligned(source_p));
  DCHECK_EQ(0u, frames_to_process % kFloatsPerPack);

  MType m_sum = MM_PS(setzero)();

  while (source_p < source_end_p) {
    MType m_source = MM_PS(load)(source_p);
    m_sum = MM_PS(add)(m_sum, MM_PS(mul)(m_source, m_source));
    source_p += kFloatsPerPack;
  }

  // Combine the packed floats.
  const float* sums = reinterpret_cast<const float*>(&m_sum);
  for (unsigned i = 0u; i < kFloatsPerPack; ++i)
    *sum_p += sums[i];
}

// real_dest_p[k] = real1[k] * real2[k] - imag1[k] * imag2[k]
// imag_dest_p[k] = real1[k] * imag2[k] + imag1[k] * real2[k]
void Zvmul(const float* real1p,
           const float* imag1p,
           const float* real2p,
           const float* imag2p,
           float* real_dest_p,
           float* imag_dest_p,
           size_t frames_to_process) {
  DCHECK(IsAligned(real1p));
  DCHECK(IsAligned(imag1p));
  DCHECK(IsAligned(real2p));
  DCHECK(IsAligned(imag2p));
  DCHECK(IsAligned(real_dest_p));
  DCHECK(IsAligned(imag_dest_p));
  DCHECK_EQ(0u, frames_to_process % kFloatsPerPack);

  for (size_t i = 0u; i < frames_to_process; i += kFloatsPerPack) {
    MType real1 = MM_PS(load)(real1p + i);
    MType real2 = MM_PS(load)(real2p + i);
    MType imag1 = MM_PS(load)(imag1p + i);
    MType imag2 = MM_PS(load)(imag2p + i);
    MType real = MM_PS(sub)(MM_PS(mul)(real1, real2), MM_PS(mul)(imag1, imag2));
    MType imag = MM_PS(add)(MM_PS(mul)(real1, imag2), MM_PS(mul)(imag1, real2));
    MM_PS(store)(real_dest_p + i, real);
    MM_PS(store)(imag_dest_p + i, imag);
  }
}

#undef MM_PS

#if !defined(__AVX__)
}  // namespace
#endif  // !defined(__AVX__)

#if defined(__AVX__)
}  // namespace AVX
#else
}  // namespace SSE
#endif

}  // namespace VectorMath
}  // namespace blink

#endif  // defined(ARCH_CPU_X86_FAMILY) && !defined(OS_MACOSX)

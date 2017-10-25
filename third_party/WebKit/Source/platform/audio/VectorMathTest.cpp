// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/audio/VectorMath.h"

#include <algorithm>
#include <array>
#include <random>

#include "base/time/time.h"
#include "platform/wtf/MathExtras.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
namespace VectorMath {
namespace {

struct MemoryLayout {
  size_t byte_alignment;
  size_t stride;
};

// This is the minimum aligned needed by AVX on x86 family architectures.
constexpr size_t kMaxBitAlignment = 256u;
constexpr size_t kMaxByteAlignment = kMaxBitAlignment / 8u;

constexpr size_t kMaxStride = 2u;

constexpr MemoryLayout kMemoryLayouts[] = {
    {kMaxByteAlignment / 2u - kMaxByteAlignment / 4u, 1u},
    {kMaxByteAlignment / 2u, 1u},
    {kMaxByteAlignment / 2u + kMaxByteAlignment / 4u, 1u},
    {kMaxByteAlignment, 1u},
    {0u, kMaxStride}};
constexpr size_t kMemoryLayoutCount =
    sizeof(kMemoryLayouts) / sizeof(*kMemoryLayouts);

// This is the minimum range size in bytes needed for MSA instructions on MIPS.
constexpr size_t kMaxRangeSizeInBytes = 1024u;
constexpr size_t kRangeSizesInBytes[] = {
    kMaxRangeSizeInBytes,
    // This range size in bytes is chosen so that the following optimization
    // paths can be tested on x86 family architectures using different memory
    // layouts:
    //  * AVX + SSE + scalar
    //  * scalar + SSE + AVX
    //  * SSE + AVX + scalar
    //  * scalar + AVX + SSE
    // On other architectures, this range size in bytes results in either
    // optimization + scalar path or scalar path to be tested.
    kMaxByteAlignment + kMaxByteAlignment / 2u + kMaxByteAlignment / 4u};
constexpr size_t kRangeSizeCount =
    sizeof(kRangeSizesInBytes) / sizeof(*kRangeSizesInBytes);

// This represents a range which is aligned and can be non-contiguous.
template <typename T>
class Range {
  class Iterator {
   public:
    using difference_type = ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using pointer = T*;
    using reference = T&;
    using value_type = T;

    Iterator(T* p, int stride) : p_(p), stride_(stride) {}

    Iterator& operator++() {
      p_ += stride_;
      return *this;
    }
    Iterator operator++(int) {
      Iterator iter = *this;
      ++(*this);
      return iter;
    }
    bool operator==(const Iterator& other) const { return p_ == other.p_; }
    bool operator!=(const Iterator& other) const { return !(*this == other); }
    T& operator*() const { return *p_; }

   private:
    T* p_;
    size_t stride_;
  };

 public:
  using const_iterator = Iterator;
  using iterator = Iterator;

  Range() = default;
  Range(T* base, const MemoryLayout* memory_layout, size_t size)
      : p_(GetAligned(base, memory_layout->byte_alignment)),
        memory_layout_(memory_layout),
        size_(size) {}
  Range(T* base, const Range<const T>& primary_range)
      : Range(base, primary_range.memory_layout(), primary_range.size()) {}

  Iterator begin() const { return Iterator(p_, stride()); }
  Iterator end() const { return Iterator(p_ + size() * stride(), stride()); }
  const MemoryLayout* memory_layout() const { return memory_layout_; }
  T* p() const { return p_; }
  size_t size() const { return size_; }
  int stride() const { return static_cast<int>(memory_layout()->stride); }

  bool operator==(const Range& other) const {
    return std::equal(begin(), end(), other.begin(), other.end());
  }
  T& operator[](size_t i) const { return p_[i * stride()]; }

 private:
  static T* GetAligned(T* base, size_t byte_alignment) {
    size_t base_byte_alignment = GetByteAlignment(base);
    size_t byte_offset =
        (byte_alignment - base_byte_alignment + kMaxByteAlignment) %
        kMaxByteAlignment;
    T* p = base + byte_offset / sizeof(T);
    size_t p_byte_alignment = GetByteAlignment(p);
    CHECK_EQ(byte_alignment % kMaxByteAlignment, p_byte_alignment);
    return p;
  }
  static size_t GetByteAlignment(T* p) {
    return reinterpret_cast<size_t>(p) % kMaxByteAlignment;
  }

  T* p_;
  const MemoryLayout* memory_layout_;
  size_t size_;
};

// Get primary ranges with difference memory layout and size combinations.
template <typename T>
std::array<Range<T>, kRangeSizeCount * kMemoryLayoutCount> GetPrimaryRanges(
    T* base) {
  std::array<Range<T>, kRangeSizeCount * kMemoryLayoutCount> ranges;
  for (auto& range : ranges) {
    ptrdiff_t i = &range - &ranges[0];
    ptrdiff_t memory_layout_index = i % kMemoryLayoutCount;
    ptrdiff_t size_index = i / kMemoryLayoutCount;
    range = Range<T>(base, &kMemoryLayouts[memory_layout_index],
                     kRangeSizesInBytes[size_index] / sizeof(T));
  }
  return ranges;
}

// Get secondary ranges. As the size of a secondary range must always be
// the same as the size of the primary range, there are only two interesting
// secondary ranges:
//  - A range with the same memory layout as the primary range has and
//    which therefore is aligned whenever the primary range is aligned.
//  - A range with a different memory layout than the primary range has and
//    which therefore is not aligned when the primary range is aligned.
template <typename T>
std::array<Range<T>, 2u> GetSecondaryRanges(
    T* base,
    const Range<const float>& primary_range) {
  std::array<Range<T>, 2u> ranges;
  const MemoryLayout* primary_memory_layout = primary_range.memory_layout();
  const MemoryLayout* other_memory_layout =
      &kMemoryLayouts[primary_memory_layout == &kMemoryLayouts[0]];
  CHECK_NE(primary_memory_layout, other_memory_layout);
  ranges[0] = Range<T>(base, primary_range);
  ranges[1] = Range<T>(base, other_memory_layout, primary_range.size());
  return ranges;
}

class VectorMathTest : public testing::Test {
 protected:
  enum {
    kFloatArraySize =
        (kMaxStride * kMaxRangeSizeInBytes + kMaxByteAlignment - 1u) /
        sizeof(float),
    kMaxDestinationCount = 4u,
    kMaxSourceCount = 4u
  };

  float* GetDestination(size_t i) {
    CHECK_LT(i, kMaxDestinationCount);
    return destinations_[i];
  }
  const float* GetSource(size_t i) {
    CHECK_LT(i, kMaxSourceCount);
    return sources_[i];
  }

  static void SetUpTestCase() {
    std::generate_n(sources_[0], sizeof(sources_) / sizeof(sources_[0][0]),
                    GetRandomFloat);
  };

 private:
  static float GetRandomFloat() {
    static std::minstd_rand generator(
        (base::Time::Now() - base::Time()).InNanoseconds());
    static std::uniform_real_distribution<float> distribution(-10.0f, 10.0f);
    return distribution(generator);
  }

  static float destinations_[kMaxDestinationCount][kFloatArraySize];
  static float sources_[kMaxSourceCount][kFloatArraySize];
};

float VectorMathTest::destinations_[kMaxDestinationCount][kFloatArraySize];
float VectorMathTest::sources_[kMaxSourceCount][kFloatArraySize];

TEST_F(VectorMathTest, Vadd) {
  for (const auto& source1 : GetPrimaryRanges(GetSource(0u))) {
    for (const auto& source2 : GetSecondaryRanges(GetSource(1u), source1)) {
      Range<float> expected_dest(GetDestination(0u), source1);
      for (size_t i = 0u; i < source1.size(); ++i)
        expected_dest[i] = source1[i] + source2[i];
      for (auto& dest : GetSecondaryRanges(GetDestination(1u), source1)) {
        Vadd(source1.p(), source1.stride(), source2.p(), source2.stride(),
             dest.p(), dest.stride(), source1.size());
        EXPECT_EQ(expected_dest, dest);
      }
    }
  }
}

TEST_F(VectorMathTest, Vclip) {
  for (const auto& source : GetPrimaryRanges(GetSource(0u))) {
    const float* thresholds = GetSource(1u);
    const float low_threshold = std::min(thresholds[0], thresholds[1]);
    const float high_threshold = std::max(thresholds[0], thresholds[1]);
    Range<float> expected_dest(GetDestination(0u), source);
    for (size_t i = 0u; i < source.size(); ++i)
      expected_dest[i] = clampTo(source[i], low_threshold, high_threshold);
    for (auto& dest : GetSecondaryRanges(GetDestination(1u), source)) {
      Vclip(source.p(), source.stride(), &low_threshold, &high_threshold,
            dest.p(), dest.stride(), source.size());
      EXPECT_EQ(expected_dest, dest);
    }
  }
}

TEST_F(VectorMathTest, Vmaxmgv) {
  const auto maxmg = [](float init, float x) {
    return std::max(init, std::abs(x));
  };
  for (const auto& source : GetPrimaryRanges(GetSource(0u))) {
    const float expected_max =
        std::accumulate(source.begin(), source.end(), 0.0f, maxmg);
    float max;
    Vmaxmgv(source.p(), source.stride(), &max, source.size());
    EXPECT_EQ(expected_max, max);
  }
}

TEST_F(VectorMathTest, Vmul) {
  for (const auto& source1 : GetPrimaryRanges(GetSource(0u))) {
    for (const auto& source2 : GetSecondaryRanges(GetSource(1u), source1)) {
      Range<float> expected_dest(GetDestination(0u), source1);
      for (size_t i = 0u; i < source1.size(); ++i)
        expected_dest[i] = source1[i] * source2[i];
      for (auto& dest : GetSecondaryRanges(GetDestination(1u), source1)) {
        Vmul(source1.p(), source1.stride(), source2.p(), source2.stride(),
             dest.p(), dest.stride(), source1.size());
        EXPECT_EQ(expected_dest, dest);
      }
    }
  }
}

TEST_F(VectorMathTest, Vsma) {
  for (const auto& source : GetPrimaryRanges(GetSource(0u))) {
    const float scale = *GetSource(1u);
    const Range<const float> dest_source(GetSource(2u), source);
    Range<float> expected_dest(GetDestination(0u), source);
    for (size_t i = 0u; i < source.size(); ++i)
      expected_dest[i] = dest_source[i] + scale * source[i];
    for (auto& dest : GetSecondaryRanges(GetDestination(1u), source)) {
      std::copy(dest_source.begin(), dest_source.end(), dest.begin());
      Vsma(source.p(), source.stride(), &scale, dest.p(), dest.stride(),
           source.size());
      EXPECT_EQ(expected_dest, dest);
    }
  }
}

TEST_F(VectorMathTest, Vsmul) {
  for (const auto& source : GetPrimaryRanges(GetSource(0u))) {
    const float scale = *GetSource(1u);
    Range<float> expected_dest(GetDestination(0u), source);
    for (size_t i = 0u; i < source.size(); ++i)
      expected_dest[i] = scale * source[i];
    for (auto& dest : GetSecondaryRanges(GetDestination(1u), source)) {
      Vsmul(source.p(), source.stride(), &scale, dest.p(), dest.stride(),
            source.size());
      EXPECT_EQ(expected_dest, dest);
    }
  }
}

TEST_F(VectorMathTest, Vsvesq) {
  const auto sqsum = [](float init, float x) { return init + x * x; };
  for (const auto& source : GetPrimaryRanges(GetSource(0u))) {
    const float expected_sum =
        std::accumulate(source.begin(), source.end(), 0.0f, sqsum);
    float sum;
    Vsvesq(source.p(), source.stride(), &sum, source.size());
    // Optimized paths in Vsvesq use parallel partial sums which may result in
    // different rounding errors than the non-partial sum algorithm used here
    // and in non-optimized paths in Vsvesq.
    EXPECT_NEAR(expected_sum, sum, 1e-6f * expected_sum);
  }
}

TEST_F(VectorMathTest, Zvmul) {
  for (const auto& real1 : GetPrimaryRanges(GetSource(0u))) {
    if (real1.stride() != 1)
      continue;
    const Range<const float> imag1(GetSource(1u), real1);
    const Range<const float> real2(GetSource(2u), real1);
    const Range<const float> imag2(GetSource(3u), real1);
    Range<float> expected_dest_real(GetDestination(0u), real1);
    Range<float> expected_dest_imag(GetDestination(1u), real1);
    for (size_t i = 0u; i < real1.size(); ++i) {
      expected_dest_real[i] = real1[i] * real2[i] - imag1[i] * imag2[i];
      expected_dest_imag[i] = real1[i] * imag2[i] + imag1[i] * real2[i];
    }
    for (auto& dest_real : GetSecondaryRanges(GetDestination(2u), real1)) {
      Range<float> dest_imag(GetDestination(3u), real1);
      ASSERT_EQ(1, dest_real.stride());
      Zvmul(real1.p(), imag1.p(), real2.p(), imag2.p(), dest_real.p(),
            dest_imag.p(), real1.size());
      EXPECT_EQ(expected_dest_real, dest_real);
      EXPECT_EQ(expected_dest_imag, dest_imag);
    }
  }
}

}  // namespace
}  // namespace VectorMath
}  // namespace blink

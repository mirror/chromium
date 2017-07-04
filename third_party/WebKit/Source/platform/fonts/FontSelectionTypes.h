/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FontSelectionTypes_h
#define FontSelectionTypes_h

#include "platform/PlatformExport.h"
#include "platform/wtf/HashTableDeletedValueType.h"
#include "platform/wtf/HashTraits.h"
#include "platform/wtf/MathExtras.h"
#include "platform/wtf/StdLibExtras.h"

namespace blink {

// Unclamped, unchecked, signed fixed-point number representing a value used for
// font variations. Sixteen bits in total, one sign bit, two fractional bits,
// means the smallest positive representable value is 0.25, the maximum
// representable value is 8191.75, and the minimum representable value is -8192.
class FontSelectionValue {
 public:
  FontSelectionValue() = default;

  // Explicit because it is lossy.
  explicit FontSelectionValue(int x) : m_backing(x * fractionalEntropy) {}

  // Explicit because it is lossy.
  explicit FontSelectionValue(float x) : m_backing(x * fractionalEntropy) {}

  // Explicit because it is lossy.
  explicit FontSelectionValue(double x) : m_backing(x * fractionalEntropy) {}

  operator float() const {
    // floats have 23 fractional bits, but only 14 fractional bits are
    // necessary, so every value can be represented losslessly.
    return m_backing / static_cast<float>(fractionalEntropy);
  }

  FontSelectionValue operator+(const FontSelectionValue other) const;
  FontSelectionValue operator-(const FontSelectionValue other) const;
  FontSelectionValue operator*(const FontSelectionValue other) const;
  FontSelectionValue operator/(const FontSelectionValue other) const;
  FontSelectionValue operator-() const;
  bool operator==(const FontSelectionValue other) const;
  bool operator!=(const FontSelectionValue other) const;
  bool operator<(const FontSelectionValue other) const;
  bool operator<=(const FontSelectionValue other) const;
  bool operator>(const FontSelectionValue other) const;
  bool operator>=(const FontSelectionValue other) const;

  int16_t RawValue() const { return m_backing; }

  static FontSelectionValue maximumValue() {
    DEFINE_STATIC_LOCAL(FontSelectionValue, maximumValue,
                        (std::numeric_limits<int16_t>::max(), RawTag::RawTag));
    return maximumValue;
    /* return FontSelectionValue(std::numeric_limits<int16_t>::max(),
     * RawTag::RawTag); */
  }

  static FontSelectionValue minimumValue() {
    DEFINE_STATIC_LOCAL(FontSelectionValue, minimumValue,
                        (std::numeric_limits<int16_t>::min(), RawTag::RawTag));
    return minimumValue;
    /* return FontSelectionValue(std::numeric_limits<int16_t>::min(),
     * RawTag::RawTag); */
  }

 private:
  enum class RawTag { RawTag };

  FontSelectionValue(int16_t rawValue, RawTag) : m_backing(rawValue) {}

  static constexpr int fractionalEntropy = 4;
  int16_t m_backing{0};
};

inline FontSelectionValue FontSelectionValue::operator+(
    const FontSelectionValue other) const {
  return FontSelectionValue(m_backing + other.m_backing, RawTag::RawTag);
}

inline FontSelectionValue FontSelectionValue::operator-(
    const FontSelectionValue other) const {
  return FontSelectionValue(m_backing - other.m_backing, RawTag::RawTag);
}

inline FontSelectionValue FontSelectionValue::operator*(
    const FontSelectionValue other) const {
  return FontSelectionValue(
      static_cast<int32_t>(m_backing) * other.m_backing / fractionalEntropy,
      RawTag::RawTag);
}

inline FontSelectionValue FontSelectionValue::operator/(
    const FontSelectionValue other) const {
  return FontSelectionValue(
      static_cast<int32_t>(m_backing) / other.m_backing * fractionalEntropy,
      RawTag::RawTag);
}

inline FontSelectionValue FontSelectionValue::operator-() const {
  return FontSelectionValue(-m_backing, RawTag::RawTag);
}

inline bool FontSelectionValue::operator==(
    const FontSelectionValue other) const {
  return m_backing == other.m_backing;
}

inline bool FontSelectionValue::operator!=(
    const FontSelectionValue other) const {
  return !operator==(other);
}

inline bool FontSelectionValue::operator<(
    const FontSelectionValue other) const {
  return m_backing < other.m_backing;
}

inline bool FontSelectionValue::operator<=(
    const FontSelectionValue other) const {
  return m_backing <= other.m_backing;
}

inline bool FontSelectionValue::operator>(
    const FontSelectionValue other) const {
  return m_backing > other.m_backing;
}

inline bool FontSelectionValue::operator>=(
    const FontSelectionValue other) const {
  return m_backing >= other.m_backing;
}

static inline FontSelectionValue ItalicThreshold() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, italicThreshold, (20));
  return italicThreshold;
}

static inline bool isItalic(FontSelectionValue fontWeight) {
  return fontWeight >= ItalicThreshold();
}

static inline FontSelectionValue FontSelectionZeroValue() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, fontSelectionZeroValue, (0));
  return fontSelectionZeroValue;
}

static inline FontSelectionValue NormalSlopeValue() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, normalSlopeValue, ());
  return normalSlopeValue;
}

static inline FontSelectionValue ItalicSlopeValue() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, italicValue, (20));
  return italicValue;
}

static inline FontSelectionValue BoldThreshold() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, boldThreshold, (600));
  return boldThreshold;
}

static inline FontSelectionValue BoldWeightValue() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, boldWeightValue, (700));
  return boldWeightValue;
}

static inline FontSelectionValue NormalWeightValue() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, normalWeightValue, (400));
  return normalWeightValue;
}

static inline FontSelectionValue LightWeightValue() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, lightWeightValue, (200));
  return lightWeightValue;
}

static inline bool isFontWeightBold(FontSelectionValue fontWeight) {
  return fontWeight >= BoldThreshold();
}

static inline FontSelectionValue WeightSearchThreshold() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, weightSearchThreshold, (500));
  return weightSearchThreshold;
}

static inline FontSelectionValue UltraCondensedWidthValue() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, ultraCondensedWidthValue, (50));
  return ultraCondensedWidthValue;
}

static inline FontSelectionValue ExtraCondensedWidthValue() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, extraCondensedWidthValue, (62.5f));
  return extraCondensedWidthValue;
}

static inline FontSelectionValue CondensedWidthValue() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, condensedWidthValue, (75));
  return condensedWidthValue;
}

static inline FontSelectionValue SemiCondensedWidthValue() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, semiCondensedWidthValue, (87.5f));
  return semiCondensedWidthValue;
}

static inline FontSelectionValue NormalWidthValue() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, normalWidthValue, (100));
  return normalWidthValue;
}

static inline FontSelectionValue SemiExpandedWidthValue() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, semiExpandedWidthValue, (112.5f));
  return semiExpandedWidthValue;
}

static inline FontSelectionValue ExpandedWidthValue() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, expandedWidthValue, (125));
  return expandedWidthValue;
}

static inline FontSelectionValue ExtraExpandedWidthValue() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, extraExpandedWidthValue, (150));
  return extraExpandedWidthValue;
}

static inline FontSelectionValue UltraExpandedWidthValue() {
  DEFINE_STATIC_LOCAL(FontSelectionValue, ultraExpandedWidthValue, (200));
  return ultraExpandedWidthValue;
}

struct FontSelectionRange {
  FontSelectionRange(FontSelectionValue single_value)
      : minimum(single_value), maximum(single_value) {}

  FontSelectionRange(FontSelectionValue minimum, FontSelectionValue maximum)
      : minimum(minimum), maximum(maximum) {}

  bool operator==(const FontSelectionRange& other) const {
    return minimum == other.minimum && maximum == other.maximum;
  }

  bool IsValid() const { return minimum <= maximum; }

  bool IsRange() const { return maximum > minimum; }

  void Expand(const FontSelectionRange& other) {
    DCHECK(other.IsValid());
    if (!IsValid()) {
      *this = other;
    } else {
      minimum = std::min(minimum, other.minimum);
      maximum = std::max(maximum, other.maximum);
    }
    DCHECK(IsValid());
  }

  bool Includes(FontSelectionValue target) const {
    return target >= minimum && target <= maximum;
  }

  uint32_t UniqueValue() const {
    return minimum.RawValue() << 16 | maximum.RawValue();
  }

  FontSelectionValue clampToRange(FontSelectionValue selection_value) const {
    if (selection_value < minimum)
      return minimum;
    if (selection_value > maximum)
      return maximum;
    return selection_value;
  }

  FontSelectionValue minimum{FontSelectionValue(1)};
  FontSelectionValue maximum{FontSelectionValue(0)};
};

struct PLATFORM_EXPORT FontSelectionRequest {
  FontSelectionRequest() = default;

  FontSelectionRequest(FontSelectionValue weight,
                       FontSelectionValue width,
                       FontSelectionValue slope)
      : weight(weight), width(width), slope(slope) {}

  unsigned GetHash() const;

  bool operator==(const FontSelectionRequest& other) const {
    return weight == other.weight && width == other.width &&
           slope == other.slope;
  }

  bool operator!=(const FontSelectionRequest& other) const {
    return !operator==(other);
  }

  FontSelectionValue weight;
  FontSelectionValue width;
  FontSelectionValue slope;
};

// Only used for HashMaps. We don't want to put the bool into
// FontSelectionRequest because FontSelectionRequest needs to be as small as
// possible because it's inside every FontDescription.
struct FontSelectionRequestKey {
  FontSelectionRequestKey() = default;

  FontSelectionRequestKey(FontSelectionRequest request) : request(request) {}

  explicit FontSelectionRequestKey(WTF::HashTableDeletedValueType)
      : isDeletedValue(true) {}

  bool IsHashTableDeletedValue() const { return isDeletedValue; }

  bool operator==(const FontSelectionRequestKey& other) const {
    return request == other.request && isDeletedValue == other.isDeletedValue;
  }

  FontSelectionRequest request;
  bool isDeletedValue{false};
};

struct PLATFORM_EXPORT FontSelectionRequestKeyHash {
  static unsigned GetHash(const FontSelectionRequestKey&);

  static bool Equal(const FontSelectionRequestKey& a,
                    const FontSelectionRequestKey& b) {
    return a == b;
  }

  static const bool safe_to_compare_to_empty_or_deleted = true;
};

struct FontSelectionCapabilities {
  FontSelectionCapabilities() = default;

  FontSelectionCapabilities(FontSelectionRange width,
                            FontSelectionRange slope,
                            FontSelectionRange weight)
      : width(width), slope(slope), weight(weight), is_deleted_value_(false) {}

  FontSelectionCapabilities(WTF::HashTableDeletedValueType)
      : is_deleted_value_(true) {}

  bool IsHashTableDeletedValue() const { return is_deleted_value_; }

  void Expand(const FontSelectionCapabilities& capabilities) {
    width.Expand(capabilities.width);
    slope.Expand(capabilities.slope);
    weight.Expand(capabilities.weight);
  }

  bool IsValid() const {
    return width.IsValid() && slope.IsValid() && weight.IsValid() &&
           !is_deleted_value_;
  }

  bool HasRange() const {
    return width.IsRange() || slope.IsRange() || weight.IsRange();
  }

  bool operator==(const FontSelectionCapabilities& other) const {
    return width == other.width && slope == other.slope &&
           weight == other.weight &&
           is_deleted_value_ == other.is_deleted_value_;
  }

  bool operator!=(const FontSelectionCapabilities& other) const {
    return !(*this == other);
  }

  FontSelectionRange width{FontSelectionZeroValue(), FontSelectionZeroValue()};
  FontSelectionRange slope{FontSelectionZeroValue(), FontSelectionZeroValue()};
  FontSelectionRange weight{FontSelectionZeroValue(), FontSelectionZeroValue()};
  bool is_deleted_value_{false};
};

struct PLATFORM_EXPORT FontSelectionCapabilitiesHash {
  static unsigned GetHash(const FontSelectionCapabilities& key);

  static bool Equal(const FontSelectionCapabilities& a,
                    const FontSelectionCapabilities& b) {
    return a == b;
  }

  static const bool safe_to_compare_to_empty_or_deleted = true;
};

}  // namespace blink

namespace WTF {

template <>
struct DefaultHash<blink::FontSelectionCapabilities> {
  STATIC_ONLY(DefaultHash);
  typedef blink::FontSelectionCapabilitiesHash Hash;
};

template <>
struct HashTraits<blink::FontSelectionCapabilities>
    : SimpleClassHashTraits<blink::FontSelectionCapabilities> {
  STATIC_ONLY(HashTraits);
};

}  // namespace WTF

// Used for clampTo for example in StyleBuilderConverter
template <>
inline blink::FontSelectionValue
defaultMinimumForClamp<blink::FontSelectionValue>() {
  return blink::FontSelectionValue::minimumValue();
}

template <>
inline blink::FontSelectionValue
defaultMaximumForClamp<blink::FontSelectionValue>() {
  return blink::FontSelectionValue::maximumValue();
}

#endif

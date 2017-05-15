/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef ASCIIFastPath_h
#define ASCIIFastPath_h

#include "platform/wtf/Alignment.h"
#include "platform/wtf/CPU.h"
#include "platform/wtf/StdLibExtras.h"
#include "platform/wtf/text/Unicode.h"
#include <stdint.h>

#if OS(MACOSX) && (CPU(X86) || CPU(X86_64))
#include <emmintrin.h>
#endif

namespace WTF {

// Assuming that a pointer is the size of a "machine word", then
// uintptr_t is an integer type that is also a machine word.
typedef uintptr_t MachineWord;
const uintptr_t kMachineWordAlignmentMask = sizeof(MachineWord) - 1;

inline bool IsAlignedToMachineWord(const void* pointer) {
  return !(reinterpret_cast<uintptr_t>(pointer) & kMachineWordAlignmentMask);
}

template <typename T>
inline T* AlignToMachineWord(T* pointer) {
  return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(pointer) &
                              ~kMachineWordAlignmentMask);
}

template <std::size_t size, typename CharacterType>
struct NonASCIIMask;
template <>
struct NonASCIIMask<4, UChar> {
  static inline uint32_t Value() { return 0xFF80FF80U; }
};
template <>
struct NonASCIIMask<4, LChar> {
  static inline uint32_t Value() { return 0x80808080U; }
};
template <>
struct NonASCIIMask<8, UChar> {
  static inline uint64_t Value() { return 0xFF80FF80FF80FF80ULL; }
};
template <>
struct NonASCIIMask<8, LChar> {
  static inline uint64_t Value() { return 0x8080808080808080ULL; }
};

template <typename CharacterType>
inline bool IsAllASCII(MachineWord word) {
  return !(word & NonASCIIMask<sizeof(MachineWord), CharacterType>::Value());
}

// Note: This function assume the input is likely all ASCII, and
// does not leave early if it is not the case.
template <typename CharacterType>
inline bool CharactersAreAllASCII(const CharacterType* characters,
                                  size_t length) {
  DCHECK_GT(length, 0u);
  MachineWord all_char_bits = 0;
  const CharacterType* end = characters + length;

  // Prologue: align the input.
  while (!IsAlignedToMachineWord(characters) && characters != end) {
    all_char_bits |= *characters;
    ++characters;
  }

  // Compare the values of CPU word size.
  const CharacterType* word_end = AlignToMachineWord(end);
  const size_t kLoopIncrement = sizeof(MachineWord) / sizeof(CharacterType);
  while (characters < word_end) {
    all_char_bits |= *(reinterpret_cast_ptr<const MachineWord*>(characters));
    characters += kLoopIncrement;
  }

  // Process the remaining bytes.
  while (characters != end) {
    all_char_bits |= *characters;
    ++characters;
  }

  MachineWord non_ascii_bit_mask =
      NonASCIIMask<sizeof(MachineWord), CharacterType>::Value();
  return !(all_char_bits & non_ascii_bit_mask);
}

template <size_t size>
struct MachineDataWord {};

template <>
struct MachineDataWord<4> {
  typedef uint32_t Type;
};

template <>
struct MachineDataWord<8> {
  typedef uint64_t Type;
};

template <typename CharacterType>
struct BitsPerChar {};

template <>
struct BitsPerChar<LChar> {
  static const size_t kBits = 8;
};

template <>
struct BitsPerChar<UChar> {
  static const size_t kBits = 16;
};

// Detects whether a string contains all ASCII alpha characters.
//
// This is specialized to machine word and character type and performs
// the comparison of a machine words' worth of characters in parallel.
template <size_t size, typename CharacterType>
class ASCIIAlphaDetector {
 public:
  using MachineDataWord = typename MachineDataWord<size>::Type;

  // Tests whether every character in `characters` is [A-Za-z].
  static inline bool All(const CharacterType* characters, size_t length) {
    const CharacterType* end = characters + length;
    MachineDataWord test = Flag();

    // Test the unaligned characters at the start of the word.
    if (!IsAlignedToMachineWord(characters)) {
      MachineDataWord buffer = Placeholder();
      while (!IsAlignedToMachineWord(characters) && characters != end) {
        buffer = buffer << BitsPerChar<CharacterType>::kBits | *characters++;
      }
      test = WordIsAllASCIIAlpha(buffer);
    }

    // Test the middle of the string, one machine word at a time.
    const CharacterType* word_end = AlignToMachineWord(end);
    const size_t kLoopIncrement = sizeof(MachineWord) / sizeof(CharacterType);
    while (characters < word_end) {
      test &= WordIsAllASCIIAlpha(
          *(reinterpret_cast_ptr<const MachineDataWord*>(characters)));
      characters += kLoopIncrement;
    }

    // Test the unaligned characters at the end of the string.
    if (characters != end) {
      MachineDataWord buffer = Placeholder();
      while (characters != end) {
        buffer = buffer << BitsPerChar<CharacterType>::kBits | *characters++;
      }
      test &= WordIsAllASCIIAlpha(buffer);
    }

    // Check that all the characters passed the tests.
    return (test & Flag()) == Flag();
  }

 private:
  // Compares characters packed into machine words a and b. The h.o.
  // bits of each must be clear. For each pair of values v_a and v_b
  // packed into a and b, if v_a < v_b then the h.o. bit of the result
  // is set. (Ignore the other bits.)
  static inline MachineDataWord LessThan(MachineDataWord a, MachineDataWord b) {
    // This works the following way; assume a and b are unsigned
    // integers < 2^n. If,
    //
    //       a <  b
    // a + 2^n <  b + 2^n
    //     2^n <= b + 2^n - a - 1
    //
    // Note 2^n - a - 1 is just `a XOR (2^n - 1)` so:
    //
    //     2^n <= b + (a XOR (2^n - 1))
    //
    // So to test a < b, compute the RHS and test whether it is at
    // least 2^n by checking the h.o. bit of the result.
    //
    // See Lamport, Leslie (1975) Multiple Byte Processing with
    // Full-Word Instructions. CACM, 18 (8) pp. 471-5.
    return b + (a ^ ~Flag());
  }

  static inline MachineDataWord WordIsAllASCIIAlpha(MachineDataWord word) {
    // The high order bit must be clear for the less-than test to work.
    MachineDataWord char_high_order_bit_clear = ~(word & Flag());
    // Assuming characters are alpha, convert them to lowercase.
    word |= ToLower();
    // 'a' <= ch
    MachineDataWord lower_bound = LessThan(Lower(), word);
    // ch <= 'z'
    MachineDataWord upper_bound = LessThan(word, Upper());
    return char_high_order_bit_clear & lower_bound & upper_bound;
  }

  // A string of 'aaa...' When examining the non-word aligned
  // characters in a string this placeholder is used to populate
  // spaces with characters which trivially pass the check.
  static inline MachineDataWord Placeholder();

  // The exclusive lower bound of the check, '```...'. Note backtick
  // (0x60) is the character immediately before 'a' (0x61) in ASCII.
  static inline MachineDataWord Lower();

  // The exclusive upper bound of the check, '{{{...'. Note left curly
  // brace (0x7b) is the character immediately after 'z' (0x7a) in
  // ASCII.
  static inline MachineDataWord Upper();

  // Given a character 'A' <= ch <= 'Z', setting bit 6 (0x20)
  // transforms it to lowercase.
  static inline MachineDataWord ToLower();

  // The flag set by logical operations; the high order bit of a
  // character.
  static inline MachineDataWord Flag();
};

// 4-byte word, UTF16
template <>
inline uint32_t ASCIIAlphaDetector<4, UChar>::Placeholder() {
  return 0x00610061U;
}
template <>
inline uint32_t ASCIIAlphaDetector<4, UChar>::Lower() {
  return 0x00600060U;
}
template <>
inline uint32_t ASCIIAlphaDetector<4, UChar>::Upper() {
  return 0x007b007bU;
}
template <>
inline uint32_t ASCIIAlphaDetector<4, UChar>::ToLower() {
  return 0x00200020U;
}
template <>
inline uint32_t ASCIIAlphaDetector<4, UChar>::Flag() {
  return 0x80008000U;
}

// 4-byte word, ASCII/UTF8
template <>
inline uint32_t ASCIIAlphaDetector<4, LChar>::Placeholder() {
  return 0x61616161U;
}
template <>
inline uint32_t ASCIIAlphaDetector<4, LChar>::Lower() {
  return 0x60606060U;
}
template <>
inline uint32_t ASCIIAlphaDetector<4, LChar>::Upper() {
  return 0x7b7b7b7bU;
}
template <>
inline uint32_t ASCIIAlphaDetector<4, LChar>::ToLower() {
  return 0x20202020U;
}
template <>
inline uint32_t ASCIIAlphaDetector<4, LChar>::Flag() {
  return 0x80808080U;
}

// 8-byte word, UTF16
template <>
inline uint64_t ASCIIAlphaDetector<8, UChar>::Placeholder() {
  return 0x0061006100610061ULL;
}
template <>
inline uint64_t ASCIIAlphaDetector<8, UChar>::Lower() {
  return 0x0060006000600060ULL;
}
template <>
inline uint64_t ASCIIAlphaDetector<8, UChar>::Upper() {
  return 0x007b007b007b007bULL;
}
template <>
inline uint64_t ASCIIAlphaDetector<8, UChar>::ToLower() {
  return 0x0020002000200020ULL;
}
template <>
inline uint64_t ASCIIAlphaDetector<8, UChar>::Flag() {
  return 0x8000800080008000ULL;
}

// 8-byte word, ASCII/UTF8
template <>
inline uint64_t ASCIIAlphaDetector<8, LChar>::Placeholder() {
  return 0x6161616161616161ULL;
}
template <>
inline uint64_t ASCIIAlphaDetector<8, LChar>::Lower() {
  return 0x6060606060606060ULL;
}
template <>
inline uint64_t ASCIIAlphaDetector<8, LChar>::Upper() {
  return 0x7b7b7b7b7b7b7b7bULL;
}
template <>
inline uint64_t ASCIIAlphaDetector<8, LChar>::ToLower() {
  return 0x2020202020202020ULL;
}
template <>
inline uint64_t ASCIIAlphaDetector<8, LChar>::Flag() {
  return 0x8080808080808080ULL;
}

inline void CopyLCharsFromUCharSource(LChar* destination,
                                      const UChar* source,
                                      size_t length) {
#if OS(MACOSX) && (CPU(X86) || CPU(X86_64))
  const uintptr_t kMemoryAccessSize =
      16;  // Memory accesses on 16 byte (128 bit) alignment
  const uintptr_t kMemoryAccessMask = kMemoryAccessSize - 1;

  size_t i = 0;
  for (; i < length && !IsAlignedTo<kMemoryAccessMask>(&source[i]); ++i) {
    DCHECK(!(source[i] & 0xff00));
    destination[i] = static_cast<LChar>(source[i]);
  }

  const uintptr_t kSourceLoadSize =
      32;  // Process 32 bytes (16 UChars) each iteration
  const size_t kUcharsPerLoop = kSourceLoadSize / sizeof(UChar);
  if (length > kUcharsPerLoop) {
    const size_t end_length = length - kUcharsPerLoop + 1;
    for (; i < end_length; i += kUcharsPerLoop) {
#if DCHECK_IS_ON()
      for (unsigned check_index = 0; check_index < kUcharsPerLoop;
           ++check_index)
        DCHECK(!(source[i + check_index] & 0xff00));
#endif
      __m128i first8u_chars =
          _mm_load_si128(reinterpret_cast<const __m128i*>(&source[i]));
      __m128i second8u_chars =
          _mm_load_si128(reinterpret_cast<const __m128i*>(&source[i + 8]));
      __m128i packed_chars = _mm_packus_epi16(first8u_chars, second8u_chars);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(&destination[i]),
                       packed_chars);
    }
  }

  for (; i < length; ++i) {
    DCHECK(!(source[i] & 0xff00));
    destination[i] = static_cast<LChar>(source[i]);
  }
#elif COMPILER(GCC) && CPU(ARM_NEON) && \
    !(CPU(BIG_ENDIAN) || CPU(MIDDLE_ENDIAN)) && defined(NDEBUG)
  const LChar* const end = destination + length;
  const uintptr_t kMemoryAccessSize = 8;

  if (length >= (2 * kMemoryAccessSize) - 1) {
    // Prefix: align dst on 64 bits.
    const uintptr_t kMemoryAccessMask = kMemoryAccessSize - 1;
    while (!IsAlignedTo<kMemoryAccessMask>(destination))
      *destination++ = static_cast<LChar>(*source++);

    // Vector interleaved unpack, we only store the lower 8 bits.
    const uintptr_t length_left = end - destination;
    const LChar* const simd_end = end - (length_left % kMemoryAccessSize);
    do {
      asm("vld2.8   { d0-d1 }, [%[SOURCE]] !\n\t"
          "vst1.8   { d0 }, [%[DESTINATION],:64] !\n\t"
          : [SOURCE] "+r"(source), [DESTINATION] "+r"(destination)
          :
          : "memory", "d0", "d1");
    } while (destination != simd_end);
  }

  while (destination != end)
    *destination++ = static_cast<LChar>(*source++);
#else
  for (size_t i = 0; i < length; ++i) {
    DCHECK(!(source[i] & 0xff00));
    destination[i] = static_cast<LChar>(source[i]);
  }
#endif
}

}  // namespace WTF

#endif  // ASCIIFastPath_h

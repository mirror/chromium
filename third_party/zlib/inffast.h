/* inffast.h -- header to use inffast.c
 * Copyright (C) 1995-2003, 2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* The TRY_INFLATE_FAST64 macro is set at configuration time, to enable testing
   if certain optimization techniques are applicable, on modern CPUs and modern
   compilers.

   One technique is INFLATE_FAST64_PULLBITS, which implements inffast.c's
   PULLBITS via 8 byte loads instead of 1 byte loads. It requires:
     - INFLATE_FAST64_UNALIGNED_LOADS_LITTLE_ENDIAN and
       INFLATE_FAST64_INLINE_MEMCPY, or
     - FORCE_INFLATE_FAST64_PULLBITS.
 */
#ifdef TRY_INFLATE_FAST64

/* The INFLATE_FAST64_PULLBITS code shows consistent improvement on x86_64 and
   is automatically enabled there. In theory, that optimization technique could
   also apply to AArch64 and PowerPC64, amongst others. In practice, there is
   possibly wider variation in how such hardware performs with unaligned loads,
   so it is disabled by default.

   Zlib builders can opt in to configuring with FORCE_INFLATE_FAST64_PULLBITS
   after running their own benchmarks on the hardware they care about to confirm
   the performance improvement.

   For reference:
   __x86_64__ is used by Clang and GCC. _M_X64 is used by MSVC.
   __ppc64__ is used by Clang. __PPC64__ is used by GCC.
   __aarch64__ is used by Clang and GCC.
 */
#if defined(__x86_64__) || defined(_M_X64)
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define INFLATE_FAST64_UNALIGNED_LOADS_LITTLE_ENDIAN 1
#endif
#endif

/* The INFLATE_FAST64_PULLBITS code loads bits into the bit accumulator
   (called hold) 8 bytes (64 bits) at a time instead of 1 byte at a time. The
   code itself doesn't do this, but conceptually, it puns a uint8_t* pointer
   (called in) as a uint64_t* pointer:
        hold |= *((uint64_t *)(in)) << bits;
   bits (the variable) is the number of bits (one eighth of a byte) that is
   currently held in hold.

   Technically speaking, this is undefined behavior according to the C
   specification. Instead, what the code actually does is to use an equivalent
   but standards compliant technique:
        uint64_t memcpy_tmp;
        memcpy(&memcpy_tmp, in, 8);
        hold |= memcpy_tmp << bits;

   This load happens inside the inflate_fast inner loop. Having this perform
   well depends on the C compiler being able to recognize that memcpy to a
   temporary as a single load instruction instead of a literal function call.

   Zlib is expected to work well across a variety of C compilers. We therefore
   disable this optimization by default, and whitelist specific compilers:
   Clang, GCC and MSC (Microsoft). Note that Clang also #define's __GNUC__ as
   it aims to be a drop-in replacement for GCC.

   Once again, zlib builders can also opt in via FORCE_INFLATE_FAST64_PULLBITS.
 */
#if defined(__GNUC__) || defined(_MSC_VER)
#define INFLATE_FAST64_INLINE_MEMCPY
#endif

#if defined(INFLATE_FAST64_UNALIGNED_LOADS_LITTLE_ENDIAN) && \
    defined(INFLATE_FAST64_INLINE_MEMCPY)
#define INFLATE_FAST64_PULLBITS 1
#endif

#endif /* TRY_INFLATE_FAST64 */

#ifdef FORCE_INFLATE_FAST64_PULLBITS
#define INFLATE_FAST64_PULLBITS 1
#endif

/* INFLATE_FAST_MIN_LEFT is the minimum number of output bytes that are left,
   so that we can call inflate_fast safely with only one up front bounds check.
   One length-distance code pair can copy up to 258 bytes.
 */
#define INFLATE_FAST_MIN_LEFT 258

/* INFLATE_FAST_MIN_HAVE is the minimum number of input bytes that we have, so
   that we can call inflate_fast safely with only one up front bounds check.
   One length-distance code pair (as two Huffman encoded values of up to 15
   bits each) plus any additional bits (up to 5 for length and 13 for distance)
   can require reading up to 48 bits, or 6 bytes.
 */
#if defined(INFLATE_FAST64_PULLBITS)
/* Round up to a multiple of 8. */
#define INFLATE_FAST_MIN_HAVE 8
#else
#define INFLATE_FAST_MIN_HAVE 6
#endif

void ZLIB_INTERNAL inflate_fast OF((z_streamp strm, unsigned start));

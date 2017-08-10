/* inffast.h -- header to use inffast.c
 * Copyright (C) 1995-2003, 2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

#if !defined(ZLIB_64_BIT_READS)

#define ZLIB_INTERNAL_INFLATE_FAST_MIN_LEFT 258
#define ZLIB_INTERNAL_INFLATE_FAST_MIN_HAVE 6

#else

/* __x86_64__ is used by Clang and GCC. _M_X64 is used by MSVC.
   __ppc64__ is used by Clang. __PPC64__ is used by GCC.
   __aarch64__ is used by Clang and GCC.
 */
#if defined(__x86_64__) || defined(_M_X64)
#define ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS 1
#define ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS_LITTLE_ENDIAN 1
#endif

#if defined(__ppc64__) || defined(__PPC__64) || defined(__aarch64__)
#define ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS 1
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS_LITTLE_ENDIAN 1
#endif
#endif

/* If using 64 bit loads, round MIN_LEFT and MIN_HAVE up to a multiple of 8. */

#ifdef ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS
#define ZLIB_INTERNAL_INFLATE_FAST_MIN_LEFT 264
#else
#define ZLIB_INTERNAL_INFLATE_FAST_MIN_LEFT 258
#endif

#ifdef ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS_LITTLE_ENDIAN
#define ZLIB_INTERNAL_INFLATE_FAST_MIN_HAVE 8
#else
#define ZLIB_INTERNAL_INFLATE_FAST_MIN_HAVE 6
#endif

#endif

void ZLIB_INTERNAL inflate_fast OF((z_streamp strm, unsigned start));

/*
 * Copyright 2007-2008 Google, Inc. All Rights Reserved.
 * <<license>>
 */

/**
 * Dtoa needs to have a particular environment set up for it so
 * instead of using it directly you should use this file.
 *
 * The way it works is that when you link with it, its definitions
 * of dtoa, strtod etc. override the default ones.  So if you fail
 * to link with this library everything will still work, it's just
 * subtly wrong.
 */

#if !(defined(__APPLE__) && defined(__MACH__)) && !defined(WIN32)
#include <endian.h>
#endif
#include <math.h>
#include <float.h>

/* The floating point word order on ARM is big endian when floating point
 * emulation is used, even if the byte order is little endian */
#if !(defined(__APPLE__) && defined(__MACH__)) && !defined(WIN32) && \
  __FLOAT_WORD_ORDER == __BIG_ENDIAN
#define  IEEE_MC68k
#else
#define  IEEE_8087
#endif

#define __MATH_H__
#if defined(__APPLE__) && defined(__MACH__)
/* stdlib.h on Apple's 10.5 and later SDKs will mangle the name of strtod.
 * If it's included after strtod is redefined as gay_strtod, it will mangle
 * the name of gay_strtod, which is unwanted. */
#include <stdlib.h>
#endif
/* Make sure we use the David M. Gay version of strtod(). On Linux, we
 * cannot use the same name (maybe the function does not have weak
 * linkage?). */
#define strtod gay_strtod
#include "third_party/dtoa/dtoa.c"

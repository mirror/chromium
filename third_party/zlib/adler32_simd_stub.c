/* zlib/adler32_simd_stub.c
 *
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include <assert.h>

#include "adler32_simd.h"
#include "deflate.h"

uint32_t ZLIB_INTERNAL adler32_simd_(
    uint32_t ladler,
    const unsigned char *buf,
    size_t len) {
    assert(0);
    return 1L;
}

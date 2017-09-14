/* adler32_simd.h
 *
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include <stdint.h>
#include <stdlib.h>

uint32_t ZLIB_INTERNAL adler32_simd_(
    uint32_t ladler,
    const unsigned char *buf,
    size_t len);

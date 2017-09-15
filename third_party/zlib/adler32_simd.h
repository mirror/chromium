/* adler32_simd.h
 *
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include <stdint.h>

#include "zconf.h"
#include "zutil.h"

uint32_t ZLIB_INTERNAL adler32_simd_(
    uint32_t adler,
    const unsigned char *buf,
    z_size_t len);

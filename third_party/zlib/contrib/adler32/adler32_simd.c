/* zlib/contrib/adler32/adler32_simd.c
 *
 * For conditions of distribution and use, see copyright notice in zlib.h
 *
 * Per http://en.wikipedia.org/wiki/Adler-32 the adler32 A value (aka s1) is
 * the sum of N input data bytes D1 ... DN,
 *
 *   A = A0 + D1 + D2 + ... + DN
 *
 * where A0 is the initial value.
 *
 * SSE2 _mm_sad_epu8() can be used for byte sums (see http://bit.ly/2wpUOeD,
 * for example) and accumulating the byte sums can use SSE shuffle-adds (see
 * the "Integer" section of http://bit.ly/2erPT8t for details). Arm NEON has
 * similar instructions.
 *
 * The adler32 B value (aka s2) sums the A values from each step:
 *
 *   B0 + (A0 + D1) + (A0 + D1 + D2) + ... + (A0 + D1 + D2 + ... + DN) or
 *
 *       B0 + N.A0 + N.D1 + (N-1).D2 + (N-2).D3 + ... + (N-(N-1)).DN
 *
 * B0 being the initial value. For 32 bytes (ideal for garden-variety SIMD):
 *
 *   B = B0 + 32.A0 + [D1 D2 D3 ... D32] x [32 31 30 ... 1].
 *
 * Adjacent blocks of 32 input bytes can be iterated with the expressions to
 * compute the adler32 s1 s2 of M >> 32 input bytes [1].
 *
 * As M grows, the s1 s2 sums grow. If left unchecked, they would eventually
 * overflow the precision of their integer representation (bad). However, s1
 * and s2 also need to be computed modulo the adler BASE value (reduced). If
 * at most NMAX bytes are processed before a reduce, s1 s2 _cannot_ overflow
 * a uint32_t type (good) [2].
 *
 * [1] the iterative equations for s2 contain constant factors; these can be
 * hoisted out of the n-blocks do loop.
 *
 * [2] zlib adler32_z() uses this fact to implement NMAX-block-based updates
 * of the adler s1 s2 of uint32_t type (refer to alder32.c).
 */

#if defined(ADLER32_SIMD)

// SSSE3

#if defined(__SSSE3__)

#include <tmmintrin.h>

static uint32_t adler32_ssse3_vector_(
    uint32_t adler,
    const unsigned char *buf,
    size_t len)
{
    /*
     * Split Adler-32 into component sums.
     */
    uint32_t s1 = adler & 0xffff;
    uint32_t s2 = adler >> 16;

    /*
     * Serially compute s1 & s2, until the data is 16-byte aligned.
     */
    if ((uintptr_t)buf & 15) {
        while ((uintptr_t)buf & 15) {
            s1 += *buf++;
            s2 += s1;
            --len;
        }

        if (s1 >= BASE)
            s1 -= BASE;
        s2 %= BASE;
    }

    /*
     * Process the data in blocks.
     */
    const unsigned BLOCK_SIZE = 1 << 5;

    size_t blocks = len / BLOCK_SIZE;
    len -= blocks * BLOCK_SIZE;

    while (blocks)
    {
        unsigned n = NMAX / BLOCK_SIZE;
        if (n > blocks)
            n = blocks;

        blocks -= n;

        typedef union { int8_t i8[16]; __m128i m; } vi8_t;

        static const vi8_t tap1 =
            { { 32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17 } };
        static const vi8_t tap2 =
            { { 16,15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1 } };

        typedef const __m128i __input;

        __input ones = _mm_set1_epi16(1);
        __input zero = _mm_set1_epi16(0);

        /*
         * Process n blocks of data. At most NMAX data bytes can be
         * processed before s2 must be reduced modulo BASE.
         */
        __m128i v_ps = _mm_set_epi32(0, 0, 0, s1 * n);
        __m128i v_s1 = zero;
        __m128i v_s2 = zero;

        do {
            /*
             * Load 32 input bytes.
             */
            __input bytes1 = _mm_load_si128((__input*)(buf));
            __input bytes2 = _mm_load_si128((__input*)(buf + 16));

            /*
             * Add previous block byte sum to v_ps.
             */
            v_ps = _mm_add_epi32(v_ps, v_s1);

            /*
             * Horizontally add the bytes for s1, multiply-adds the
             * bytes by [ 32, 31, 30, ... ] for s2.
             */
            v_s1 = _mm_add_epi32(v_s1, _mm_sad_epu8(bytes1, zero));
            const __m128i mad1 = _mm_maddubs_epi16(bytes1, tap1.m);
            v_s2 = _mm_add_epi32(v_s2, _mm_madd_epi16(mad1, ones));

            v_s1 = _mm_add_epi32(v_s1, _mm_sad_epu8(bytes2, zero));
            const __m128i mad2 = _mm_maddubs_epi16(bytes2, tap2.m);
            v_s2 = _mm_add_epi32(v_s2, _mm_madd_epi16(mad2, ones));

            buf += BLOCK_SIZE;

        } while (--n);

        v_s2 = _mm_add_epi32(v_s2, _mm_slli_epi32(v_ps, 5));

        /*
         * Sum epi32 ints v_s1(s2) and accumulate in s1(s2).
         */

#define S23O1 _MM_SHUFFLE(2,3,0,1)  /* A B C D -> B A D C */
#define S1O32 _MM_SHUFFLE(1,0,3,2)  /* A B C D -> C D A B */

        v_s1 = _mm_add_epi32(v_s1, _mm_shuffle_epi32(v_s1, S23O1));
        v_s1 = _mm_add_epi32(v_s1, _mm_shuffle_epi32(v_s1, S1O32));

        s1 += _mm_cvtsi128_si32(v_s1);

        v_s2 = _mm_add_epi32(v_s2, _mm_shuffle_epi32(v_s2, S23O1));
        v_s2 = _mm_add_epi32(v_s2, _mm_shuffle_epi32(v_s2, S1O32));

        s2 += _mm_cvtsi128_si32(v_s2);

        /*
         * Reduce.
         */
        s1 %= BASE;
        s2 %= BASE;
    }

    /*
     * Handle leftover data.
     */
    if (len) {
        while (len >= 16) {
            for (unsigned i = 4; i; --i) {
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
                s2 += (s1 += *buf++);
            }
            len -= 16;
        }

        while (len >= 4) {
            s2 += (s1 += *buf++);
            s2 += (s1 += *buf++);
            s2 += (s1 += *buf++);
            s2 += (s1 += *buf++);
            len -= 4;
        }

        while (len--) {
           s2 += (s1 += *buf++);
        }

        if (s1 >= BASE)
            s1 -= BASE;
        s2 %= BASE;
    }

    return s1 | (s2 << 16);
}

#endif // __SSSE3_

static uint32_t adler32_simd_(
    uint32_t ladler,
    const unsigned char *buf,
    size_t len)
{
    uint32_t adler = ladler;
    uint32_t sum2;

    /*
     * split Adler-32 into component sums
     */
    sum2 = adler >> 16;
    adler &= 0xffff;

    /*
     * In case user likes doing a byte at a time, keep it fast
     */
    if (len == 1) {
        adler += buf[0];
        if (adler >= BASE)
            adler -= BASE;
        sum2 += adler;
        if (sum2 >= BASE)
            sum2 -= BASE;
        return adler | (sum2 << 16);
    }

    /*
     * Initial Adler-32 value (deferred check for len == 1 speed)
     */
    if (buf == NULL)
        return 1L;

    /*
     * In case short lengths are provided, keep it somewhat fast
     */
    if (len < 48) {
        while (len >= 16) {
            for (unsigned i = 4; i; --i) {
                sum2 += (adler += *buf++);
                sum2 += (adler += *buf++);
                sum2 += (adler += *buf++);
                sum2 += (adler += *buf++);
            }
            len -= 16;
        }

        while (len >= 4) {
            sum2 += (adler += *buf++);
            sum2 += (adler += *buf++);
            sum2 += (adler += *buf++);
            sum2 += (adler += *buf++);
            len -= 4;
        }

        while (len--) {
            sum2 += (adler += *buf++);
        }

        if (adler >= BASE)
            adler -= BASE;
        sum2 %= BASE;

        return adler | (sum2 << 16);
    }

    /*
     * For larger lengths, vectorize
     */
#if defined(__SSSE3__)
    return adler32_ssse3_vector_(ladler, buf, len);
#endif
}

#endif // ADLER32_SIMD

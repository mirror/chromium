/* inffast.c -- fast decoding
 * Copyright (C) 1995-2017 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "zutil.h"
#include "inftrees.h"
#include "inflate.h"
#include "inffast.h"

#ifdef ASMINF
#  pragma message("Assembler code may have bugs -- use at your own risk")
#else

#if defined(INFLATE_FAST64_PULLBITS)
#include <stdint.h>
#include <string.h>

/* PULLBITS_48 pulls at least 48 bits into the bit accumulator.

   The three-line memcpy_tmp dance here is a C standards compliant way of type
   punning a uint8_t* as a uint64_t*. It is essentially:
        hold |= *((uint64_t *)(in)) << bits;
   The memcpy function call looks superficially inefficient, but modern
   compilers like clang and gcc will generate efficient code.

   The fact that in and bits are incremented by 6 and 48 instead of 8 and 64
   is discussed below, in the comment above "uint64_t hold".
 */
#define PULLBITS_48() \
    do { \
        uint64_t memcpy_tmp; \
        memcpy(&memcpy_tmp, in, 8); \
        hold |= memcpy_tmp << bits; \
        in += 6; \
        bits += 48; \
    } while (0)

#else /* INFLATE_FAST64_PULLBITS */

/* PULLBITS_8 pulls exactly 8 bits into the bit accumulator.

   TODO: replace "hold += etc" with "hold |= etc", in PULLBITS_8, to be
   consistent with PULLBITS_48. This is only a comment for now so that the
   commit that introduced this comment has no effect whatsoever when
   INFLATE_FAST64_PULLBITS is undefined. */
#define PULLBITS_8() \
    hold += (unsigned long)(*in++) << bits; \
    bits += 8;

#endif /* INFLATE_FAST64_PULLBITS */

/*
   Decode literal, length, and distance codes and write out the resulting
   literal and match bytes until either not enough input or output is
   available, an end-of-block is encountered, or a data error is encountered.
   When large enough input and output buffers are supplied to inflate(), for
   example, a 16K input buffer and a 64K output buffer, more than 95% of the
   inflate execution time is spent in this routine.

   Entry assumptions:

        state->mode == LEN
        strm->avail_in >= INFLATE_FAST_MIN_HAVE
        strm->avail_out >= INFLATE_FAST_MIN_LEFT
        start >= strm->avail_out
        state->bits < 8
        (state->hold >> state->bits) == 0

   On return, state->mode is one of:

        LEN -- ran out of enough output space or enough available input
        TYPE -- reached end of block code, inflate() to interpret next block
        BAD -- error in block data

   Notes:

    - The maximum input bits used by a length/distance pair is 15 bits for the
      length code, 5 bits for the length extra, 15 bits for the distance code,
      and 13 bits for the distance extra.  This totals 48 bits, or six bytes.
      Therefore if strm->avail_in >= 6, then there is enough input to avoid
      checking for available input while decoding.

    - On some architectures, it can be noticably faster (e.g. up to 1.15x faster
      on x86_64) to load from strm->next_in 64 bits, or 8 bytes, at a time, so
      INFLATE_FAST_MIN_HAVE == 8. This requires a little endian architecture.

    - The maximum bytes that a single length/distance pair can output is 258
      bytes, which is the maximum length that can be coded.  inflate_fast()
      requires strm->avail_out >= 258 for each loop to avoid checking for
      output space.
 */
void ZLIB_INTERNAL inflate_fast(strm, start)
z_streamp strm;
unsigned start;         /* inflate()'s starting value for strm->avail_out */
{
    struct inflate_state FAR *state;
    z_const unsigned char FAR *in;      /* local strm->next_in */
    z_const unsigned char FAR *last;    /* have enough input while in < last */
    unsigned char FAR *out;     /* local strm->next_out */
    unsigned char FAR *beg;     /* inflate()'s initial strm->next_out */
    unsigned char FAR *end;     /* while out < end, enough space available */
#ifdef INFLATE_STRICT
    unsigned dmax;              /* maximum distance from zlib header */
#endif
    unsigned wsize;             /* window size or zero if not using window */
    unsigned whave;             /* valid bytes in the window */
    unsigned wnext;             /* window write index */
    unsigned char FAR *window;  /* allocated sliding window, if wsize != 0 */

    /* hold is a local copy of strm->hold. By default, hold satisfies the same
       invariants that strm->hold does, namely that (hold >> bits) == 0. This
       invariant is kept by loading bits into hold one byte at a time, like:

       hold |= next_byte_of_input << bits; in++; bits += 8;

       If we need to ensure that bits >= 15 then this code snippet is simply
       repeated. Over one iteration of the outermost do/while loop, this
       happens up to six times (48 bits of input), as described in the NOTES
       above.

       However, on some little endian architectures, it can be noticably faster
       to load 64 bits once instead of 8 bits six times:

       if (bits <= 16) {
         hold |= next_8_bytes_of_input << bits; in += 6; bits += 48;
       }

       Unlike the simpler one byte load, shifting the next_8_bytes_of_input
       by bits will overflow and lose those high bits, up to 2 bytes' worth.
       The conservative estimate is therefore that we have read only 6 bytes
       (48 bits). Again, as per the NOTES above, 48 bits is sufficient for the
       rest of the iteration, and we will not need to load another 8 bytes.

       Inside this function, we no longer satisfy (hold >> bits) == 0, but
       this is not problematic, even if that overflow does not land on an 8 bit
       byte boundary. Those excess bits will eventually shift down lower as the
       Huffman decoder consumes input, and when new input bits need to be loaded
       into the bits variable, the same input bits will be or'ed over those
       existing bits. A bitwise or is idempotent: (a | b | b) equals (a | b).
       Note that we therefore write that load operation as "hold |= etc" and not
       "hold += etc".

       Outside that loop, at the end of the function, hold is bitwise and'ed
       with (1<<bits)-1 to drop those excess bits so that, on function exit, we
       keep the invariant that (state->hold >> state->bits) == 0.

       TODO: rename the bits variable to nbits, so that this block comment
       is less confusing when discussing bits (the variable) and bits (one
       eighth of a byte).
       */
#ifdef INFLATE_FAST64_PULLBITS
    uint64_t hold;
#else
    unsigned long hold;
#endif

    unsigned bits;              /* local strm->bits */
    code const FAR *lcode;      /* local strm->lencode */
    code const FAR *dcode;      /* local strm->distcode */
    unsigned lmask;             /* mask for first level of length codes */
    unsigned dmask;             /* mask for first level of distance codes */
    code here;                  /* retrieved table entry */
    unsigned op;                /* code bits, operation, extra bits, or */
                                /*  window position, window bytes to copy */
    unsigned len;               /* match length, unused bytes */
    unsigned dist;              /* match distance */
    unsigned char FAR *from;    /* where to copy match from */

    /* copy state to local variables */
    state = (struct inflate_state FAR *)strm->state;
    in = strm->next_in;
    last = in + (strm->avail_in - (INFLATE_FAST_MIN_HAVE - 1));
    out = strm->next_out;
    beg = out - (start - strm->avail_out);
    end = out + (strm->avail_out - (INFLATE_FAST_MIN_LEFT - 1));
#ifdef INFLATE_STRICT
    dmax = state->dmax;
#endif
    wsize = state->wsize;
    whave = state->whave;
    wnext = state->wnext;
    window = state->window;
    hold = state->hold;
    bits = state->bits;
    lcode = state->lencode;
    dcode = state->distcode;
    lmask = (1U << state->lenbits) - 1;
    dmask = (1U << state->distbits) - 1;

    /* decode literals and length/distances until end-of-block or not enough
       input data or output space */
    do {
        if (bits < 15) {
#ifdef INFLATE_FAST64_PULLBITS
            PULLBITS_48();
#else
            PULLBITS_8();
            PULLBITS_8();
#endif
        }
        here = lcode[hold & lmask];
      dolen:
        op = (unsigned)(here.bits);
        hold >>= op;
        bits -= op;
        op = (unsigned)(here.op);
        if (op == 0) {                          /* literal */
            Tracevv((stderr, here.val >= 0x20 && here.val < 0x7f ?
                    "inflate:         literal '%c'\n" :
                    "inflate:         literal 0x%02x\n", here.val));
            *out++ = (unsigned char)(here.val);
        }
        else if (op & 16) {                     /* length base */
            len = (unsigned)(here.val);
            op &= 15;                           /* number of extra bits */
            if (op) {
                if (bits < op) {
#ifdef INFLATE_FAST64_PULLBITS
                    PULLBITS_48();
#else
                    PULLBITS_8();
#endif
                }
                len += (unsigned)hold & ((1U << op) - 1);
                hold >>= op;
                bits -= op;
            }
            Tracevv((stderr, "inflate:         length %u\n", len));
            if (bits < 15) {
#ifdef INFLATE_FAST64_PULLBITS
                PULLBITS_48();
#else
                PULLBITS_8();
                PULLBITS_8();
#endif
            }
            here = dcode[hold & dmask];
          dodist:
            op = (unsigned)(here.bits);
            hold >>= op;
            bits -= op;
            op = (unsigned)(here.op);
            if (op & 16) {                      /* distance base */
                dist = (unsigned)(here.val);
                op &= 15;                       /* number of extra bits */
                if (bits < op) {
#ifdef INFLATE_FAST64_PULLBITS
                    PULLBITS_48();
#else
                    PULLBITS_8();
                    if (bits < op) {
                        PULLBITS_8();
                    }
#endif
                }
                dist += (unsigned)hold & ((1U << op) - 1);
#ifdef INFLATE_STRICT
                if (dist > dmax) {
                    strm->msg = (char *)"invalid distance too far back";
                    state->mode = BAD;
                    break;
                }
#endif
                hold >>= op;
                bits -= op;
                Tracevv((stderr, "inflate:         distance %u\n", dist));
                op = (unsigned)(out - beg);     /* max distance in output */
                if (dist > op) {                /* see if copy from window */
                    op = dist - op;             /* distance back in window */
                    if (op > whave) {
                        if (state->sane) {
                            strm->msg =
                                (char *)"invalid distance too far back";
                            state->mode = BAD;
                            break;
                        }
#ifdef INFLATE_ALLOW_INVALID_DISTANCE_TOOFAR_ARRR
                        if (len <= op - whave) {
                            do {
                                *out++ = 0;
                            } while (--len);
                            continue;
                        }
                        len -= op - whave;
                        do {
                            *out++ = 0;
                        } while (--op > whave);
                        if (op == 0) {
                            from = out - dist;
                            do {
                                *out++ = *from++;
                            } while (--len);
                            continue;
                        }
#endif
                    }
                    from = window;
                    if (wnext == 0) {           /* very common case */
                        from += wsize - op;
                        if (op < len) {         /* some from window */
                            len -= op;
                            do {
                                *out++ = *from++;
                            } while (--op);
                            from = out - dist;  /* rest from output */
                        }
                    }
                    else if (wnext < op) {      /* wrap around window */
                        from += wsize + wnext - op;
                        op -= wnext;
                        if (op < len) {         /* some from end of window */
                            len -= op;
                            do {
                                *out++ = *from++;
                            } while (--op);
                            from = window;
                            if (wnext < len) {  /* some from start of window */
                                op = wnext;
                                len -= op;
                                do {
                                    *out++ = *from++;
                                } while (--op);
                                from = out - dist;      /* rest from output */
                            }
                        }
                    }
                    else {                      /* contiguous in window */
                        from += wnext - op;
                        if (op < len) {         /* some from window */
                            len -= op;
                            do {
                                *out++ = *from++;
                            } while (--op);
                            from = out - dist;  /* rest from output */
                        }
                    }
                    while (len > 2) {
                        *out++ = *from++;
                        *out++ = *from++;
                        *out++ = *from++;
                        len -= 3;
                    }
                    if (len) {
                        *out++ = *from++;
                        if (len > 1)
                            *out++ = *from++;
                    }
                }
                else {
                    from = out - dist;          /* copy direct from output */
                    do {                        /* minimum length is three */
                        *out++ = *from++;
                        *out++ = *from++;
                        *out++ = *from++;
                        len -= 3;
                    } while (len > 2);
                    if (len) {
                        *out++ = *from++;
                        if (len > 1)
                            *out++ = *from++;
                    }
                }
            }
            else if ((op & 64) == 0) {          /* 2nd level distance code */
                here = dcode[here.val + (hold & ((1U << op) - 1))];
                goto dodist;
            }
            else {
                strm->msg = (char *)"invalid distance code";
                state->mode = BAD;
                break;
            }
        }
        else if ((op & 64) == 0) {              /* 2nd level length code */
            here = lcode[here.val + (hold & ((1U << op) - 1))];
            goto dolen;
        }
        else if (op & 32) {                     /* end-of-block */
            Tracevv((stderr, "inflate:         end of block\n"));
            state->mode = TYPE;
            break;
        }
        else {
            strm->msg = (char *)"invalid literal/length code";
            state->mode = BAD;
            break;
        }
    } while (in < last && out < end);

    /* return unused bytes (on entry, bits < 8, so in won't go too far back) */
    len = bits >> 3;
    in -= len;
    bits -= len << 3;
    hold &= (1U << bits) - 1;

    /* update state and return */
    strm->next_in = in;
    strm->next_out = out;
    strm->avail_in = (unsigned)(in < last ?
        (INFLATE_FAST_MIN_HAVE - 1) + (last - in) :
        (INFLATE_FAST_MIN_HAVE - 1) - (in - last));
    strm->avail_out = (unsigned)(out < end ?
        (INFLATE_FAST_MIN_LEFT - 1) + (end - out) :
        (INFLATE_FAST_MIN_LEFT - 1) - (out - end));
    state->hold = hold;
    state->bits = bits;
    return;
}

/*
   inflate_fast() speedups that turned out slower (on a PowerPC G3 750CXe):
   - Using bit fields for code structure
   - Different op definition to avoid & for extra bits (do & for table bits)
   - Three separate decoding do-loops for direct, window, and wnext == 0
   - Special case for distance > 1 copies to do overlapped load and store copy
   - Explicit branch predictions (based on measured branch probabilities)
   - Deferring match copy and interspersed it with decoding subsequent codes
   - Swapping literal/length else
   - Swapping window/direct else
   - Larger unrolled copy loops (three is about right)
   - Moving len -= 3 statement into middle of loop
 */

#endif /* !ASMINF */

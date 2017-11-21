/* inffast.h -- header to use inffast.c
 * Copyright (C) 1995-2003, 2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

#ifndef INFFAST_CHUNK_H
#define INFFAST_CHUNK_H

void ZLIB_INTERNAL inflate_fast_chunk_ OF((z_streamp strm, unsigned start));

#endif /* INFFAST_CHUNK_H */

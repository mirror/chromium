This code speeds up inflate_fast by up to 1.5x on 64bit architectures.

This directory contains wholesale replacements for the equivalent inf*.{c,h}
files in the top level of the zlib repository.


BUILD

To adjust your build system to pick up this optimization, don't build:
  infback.c
  inffast.c
  inffast.h
  inflate.c
but instead, build:
  contrib/inffast64/infback.c
  contrib/inffast64/inffast.c
  contrib/inffast64/inffast.h
  contrib/inffast64/inflate.c

You are free to gate that build system change on the target architecture, but
that should be unnecessary if you're building e.g. 32-bit x86. The
contrib/inffast64 flavors are designed to be equivalent (via #ifdef's) to the
top level flavors for architectures other than AArch64, PowerPC64 and x86_64.


MECHANISM

There are two specific techniques that speed up zlib decompression.

One is that, when performing "copy LENGTH bytes from previously decompressed
output, DIST bytes ago", that copy of LENGTH bytes can often be done 8 bytes at
a time, instead of 1 byte at a time. This technique requires 64-bit unaligned
loads and stores. On x86_64, this can give up to a 1.3x speed up.

Two is that, when filling the bit buffer for Huffman decoding, that buffer can
be filled 8 bytes at a time, instead of 1 byte at a time. This technique
requires 64-bit unaligned and also little-endian loads. On x86_64, this can
give up to a 1.2x speed up.

Together, this change can give up to a 1.5x speed up on x86_64.

Benchmark details are at
https://bugs.chromium.org/p/chromium/issues/detail?id=760853#c26

Preliminary numbers for AArch64 and PowerPC64 both show up to a 1.2x speed up.


DIFF

The diff between the top level and this directory is essentially
https://gist.github.com/nigeltao/2884d66f16fd3109184967a78cfea7a5

The new code is structured as copy/pasted files under contrib/inffast64, even
though the patches to infback.c and inflate.c are tiny, so that there are
literally no changes to the top level C code.

TODO: update this link to be a gist of the actual diff, once the Chromium
third_party/zlib reviewers are happy with the final code.


CHROMIUM-SPECIFIC NOTES

The upstream pull request is https://github.com/madler/zlib/pull/292

The change was discussed at
https://chromium-review.googlesource.com/c/chromium/src/+/601694
and at
https://bugs.chromium.org/p/chromium/issues/detail?id=760853

Via third_party/zlib/BUILD.gn, we only enable this change on x86_64. For ARM64,
a separate patch takes precedence. For PowerPC64, we have some preliminary
performance data but not a lot, and Chrome doesn't actually ship on PowerPC64.

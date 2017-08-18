// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/win/src/heap_helper.h"

#include <windows.h>

#include "base/memory/ref_counted.h"
#include "base/win/windows_version.h"

namespace sandbox {
namespace {
#pragma pack(1)

// These are undocumented, but readily found on the internet.
#define HEAP_CLASS_8 0x00008000  // CSR port heap
#define HEAP_CLASS_MASK 0x0000f000

#define HEAP_SEGMENT_SIGNATURE 0xffeeffee
#define HEAP_SIGNATURE 0xeeffeeff

typedef struct _HEAP_ENTRY {
  PVOID Data1;
  PVOID Data2;
} HEAP_ENTRY, *PHEAP_ENTRY;

#define HEAP_HEADER_VALIDATE_LENGTH_32 0x248
#define HEAP_HEADER_VALIDATE_LENGTH_64 0x298
#define FLAGS_OFFSET_32 0x40
#define FLAGS_OFFSET_64 0x70
#define HEADER_SIGNATURE_OFFSET_32 0x60
#define HEADER_SIGNATURE_OFFSET_64 0x98
#define HEADER_VALIDATE_LENGTH_OFFSET_32 0x7e
#define HEADER_VALIDATE_LENGTH_OFFSET_64 0xd2

#if defined(_WIN64)
#define HEAP_HEADER_VALIDATE_LENGTH HEAP_HEADER_VALIDATE_LENGTH_64
#define FLAGS_OFFSET FLAGS_OFFSET_64
#define HEADER_SIGNATURE_OFFSET HEADER_SIGNATURE_OFFSET_64
#define HEADER_VALIDATE_LENGTH_OFFSET HEADER_VALIDATE_LENGTH_OFFSET_64
#else  // defined(_WIN64)
#define HEAP_HEADER_VALIDATE_LENGTH HEAP_HEADER_VALIDATE_LENGTH_32
#define FLAGS_OFFSET FLAGS_OFFSET_32
#define HEADER_SIGNATURE_OFFSET HEADER_SIGNATURE_OFFSET_32
#define HEADER_VALIDATE_LENGTH_OFFSET HEADER_VALIDATE_LENGTH_OFFSET_32
#endif  // defined(_WIN64)

// Thes structure is not documented, so only define the fields that are
// relevant.
typedef struct _HEAP_SEGMENT {
  HEAP_ENTRY HeapEntry;
  DWORD SegmentSignature;
  DWORD SegmentFlags;
  LIST_ENTRY SegmentListEntry;
  struct _HEAP* Heap;
} HEAP_SEGMENT, *PHEAP_SEGMENT;

struct _HEAP {
  _HEAP_SEGMENT HeapSegment;
  char Reserved0[FLAGS_OFFSET - sizeof(_HEAP_SEGMENT)];
  DWORD Flags;
  char Reserved1[HEADER_SIGNATURE_OFFSET - (FLAGS_OFFSET + sizeof(Flags))];
  DWORD Signature;
  char Reserved2[HEADER_VALIDATE_LENGTH_OFFSET -
                 (HEADER_SIGNATURE_OFFSET + sizeof(Signature))];
  DWORD header_validate_length;
  // Other stuff that is not relevant.
};

bool ValidateHeap(_HEAP* heap) {
  _HEAP_SEGMENT* heap_segment = &heap->HeapSegment;
  if (heap_segment->SegmentSignature != HEAP_SEGMENT_SIGNATURE) {
    return false;
  }
  if (heap_segment->Heap != heap) {
    return false;
  }
  if (heap->Signature != HEAP_SIGNATURE) {
    return false;
  }
  return true;
}
}  // namespace

bool HeapFlags(HANDLE handle, DWORD* flags) {
  if (!handle || !flags) {
    // This is an error.
    return false;
  }
  _HEAP* heap = reinterpret_cast<_HEAP*>(handle);
  if (!ValidateHeap(heap)) {
    LOG(ERROR) << "unable to validate heap";
    return false;
  }
  *flags = heap->Flags;
  return true;
}

HANDLE FindCsrPortHeap() {
  if (base::win::GetVersion() < base::win::VERSION_WIN10) {
    // This functionality has not been verified on versions before Win10.
    return nullptr;
  }
  DWORD number_of_heaps = ::GetProcessHeaps(0, NULL);
  std::unique_ptr<HANDLE[]> all_heaps(new HANDLE[number_of_heaps]);
  if (::GetProcessHeaps(number_of_heaps, all_heaps.get()) != number_of_heaps)
    return nullptr;

  // Search for the CSR port heap handle, identified purely based on flags.
  HANDLE csr_port_heap = nullptr;
  for (size_t i = 0; i < number_of_heaps; ++i) {
    HANDLE handle = all_heaps[i];
    DWORD flags = 0;
    if (!HeapFlags(handle, &flags)) {
      DLOG(ERROR) << "Unable to get flags for this heap";
      continue;
    }
    if ((flags & HEAP_CLASS_MASK) == HEAP_CLASS_8) {
      if (nullptr != csr_port_heap) {
        DLOG(ERROR) << "Found multiple suitable CSR Port heaps";
        return nullptr;
      }
      csr_port_heap = handle;
    }
  }
  return csr_port_heap;
}

}  // namespace sandbox

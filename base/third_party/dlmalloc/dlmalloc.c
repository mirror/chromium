/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "dlmalloc.h"

#include <android/log.h>
#include <sys/mman.h>
#include <sys/prctl.h>

// This is only supported by Android kernels, so it's not in the uapi headers.
#define PR_SET_VMA   0x53564d41
#define PR_SET_VMA_ANON_NAME    0

static void dlmalloc_heap_corruption_error(const char* function) {
  __android_log_print(
      ANDROID_LOG_FATAL, "DLMALLOC",
      "heap corruption detected by %s", function);
}

static void dlmalloc_heap_usage_error(const char* function, void* address) {
  __android_log_print(
      ANDROID_LOG_FATAL, "DLMALLOC",
      "invalid address or address of corrupt block %p passed to %s",
      address, function);
}

static void* dlmalloc_named_mmap(size_t length) {
  void* map = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if (map == MAP_FAILED) {
    return map;
  }
  prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, map, length, "dlmalloc");
  return map;
}

#define PROCEED_ON_ERROR 0
#define CORRUPTION_ERROR_ACTION(m) dlmalloc_heap_corruption_error(__FUNCTION__)
#define USAGE_ERROR_ACTION(m,p) dlmalloc_heap_usage_error(__FUNCTION__, p)
#define MMAP(s) dlmalloc_named_mmap(s)
#define DIRECT_MMAP(s) dlmalloc_named_mmap(s)

#define mallinfo dlmallinfo
#ifndef STRUCT_MALLINFO_DECLARED
#define STRUCT_MALLINFO_DECLARED
#endif

#include "upstream-dlmalloc/malloc.c"

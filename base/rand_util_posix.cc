// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/rand_util.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/posix/eintr_wrapper.h"
#include "base/threading/thread_local.h"

namespace {

// The number of random bytes to cache in TLS when reading small chunks of
// random numbers.
constexpr size_t kTlsCacheSize = 4096;

// We keep the file descriptor for /dev/urandom around so we don't need to
// reopen it (which is expensive), and since we may not even be able to reopen
// it if we are later put in a sandbox. This class wraps the file descriptor so
// we can use LazyInstance to handle opening it on the first access.
class URandomFd {
 public:
#if defined(OS_AIX)
  // AIX has no 64-bit support for open falgs such as -
  //  O_CLOEXEC, O_NOFOLLOW and O_TTY_INIT
  URandomFd() : fd_(HANDLE_EINTR(open("/dev/urandom", O_RDONLY))) {
    DCHECK_GE(fd_, 0) << "Cannot open /dev/urandom: " << errno;
  }
#else
  URandomFd() : fd_(HANDLE_EINTR(open("/dev/urandom", O_RDONLY | O_CLOEXEC))) {
    DCHECK_GE(fd_, 0) << "Cannot open /dev/urandom: " << errno;
  }
#endif

  ~URandomFd() { close(fd_); }

  int fd() const { return fd_; }

 private:
  const int fd_;
};

base::LazyInstance<URandomFd>::Leaky g_urandom_fd = LAZY_INSTANCE_INITIALIZER;

class RandomnessCache {
 public:
  RandomnessCache() : urandom_fd_(g_urandom_fd.Pointer()->fd()) {}
  ~RandomnessCache() = default;

  size_t num_cached_bytes_remaining() const {
    return kTlsCacheSize - cache_index_;
  }

  const void* next_cached_bytes() const { return &cache_[cache_index_]; }

  void GetRandomBytes(void* output, size_t output_length) {
    size_t num_cached_bytes = num_cached_bytes_remaining();
    if (output_length < num_cached_bytes) {
      memcpy(output, next_cached_bytes(), output_length);
      cache_index_ += output_length;
      return;
    }

    // Copy the remainder of whatever's cached.
    memcpy(output, next_cached_bytes(), num_cached_bytes);
    cache_index_ = kTlsCacheSize;

    size_t additional_bytes_needed = output_length - num_cached_bytes;
    if (additional_bytes_needed < kTlsCacheSize) {
      // If the number of bytes we're asking for is smaller than the cache size,
      // go ahead and repopulate the cache now, copying the first
      // |additional_bytes_needed| to the caller.
      const bool success = base::ReadFromFD(urandom_fd_, cache_, kTlsCacheSize);
      CHECK(success);
      cache_index_ = additional_bytes_needed;
      memcpy(static_cast<char*>(output) + num_cached_bytes, cache_,
             additional_bytes_needed);
    } else {
      // Otherwise just read from the FD directly and leave the cache empty.
      const bool success = base::ReadFromFD(
          urandom_fd_, static_cast<char*>(output) + num_cached_bytes,
          additional_bytes_needed);
      CHECK(success);
    }
  }

 private:
  const int urandom_fd_;
  char cache_[kTlsCacheSize];
  size_t cache_index_ = kTlsCacheSize;

  DISALLOW_COPY_AND_ASSIGN(RandomnessCache);
};

}  // namespace

namespace base {

// NOTE: This function must be cryptographically secure. http://crbug.com/140076
uint64_t RandUint64() {
  uint64_t number;
  RandBytes(&number, sizeof(number));
  return number;
}

void RandBytes(void* output, size_t output_length) {
  static auto* cache_tls = new base::ThreadLocalPointer<RandomnessCache>();
  RandomnessCache* cache = cache_tls->Get();
  if (!cache) {
    cache = new RandomnessCache;
    cache_tls->Set(cache);
  }
  cache->GetRandomBytes(output, output_length);
}

int GetUrandomFD(void) {
  return g_urandom_fd.Pointer()->fd();
}

}  // namespace base

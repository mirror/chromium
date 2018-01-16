// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_system.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include "crazy_linker_util.h"

// Note: unit-testing support files are in crazy_linker_files_mock.cpp

namespace crazy {

#ifndef UNIT_TESTS

bool FileDescriptor::OpenReadOnly(const char* path) {
  Close();
  fd_ = TEMP_FAILURE_RETRY(::open(path, O_RDONLY));
  return (fd_ != -1);
}

bool FileDescriptor::OpenReadWrite(const char* path) {
  Close();
  fd_ = TEMP_FAILURE_RETRY(::open(path, O_RDWR));
  return (fd_ != -1);
}

int FileDescriptor::Read(void* buffer, size_t buffer_size) {
  return TEMP_FAILURE_RETRY(::read(fd_, buffer, buffer_size));
}

int FileDescriptor::SeekTo(off_t offset) {
  return ::lseek(fd_, offset, SEEK_SET);
}

void* FileDescriptor::Map(void* address,
                          size_t length,
                          int prot,
                          int flags,
                          off_t offset) {
  return ::mmap(address, length, prot, flags, fd_, offset);
}

void FileDescriptor::Close() {
  if (fd_ != -1) {
    int old_errno = errno;
    close(fd_);
    errno = old_errno;
    fd_ = -1;
  }
}

const char* GetEnv(const char* var_name) { return ::getenv(var_name); }

String GetCurrentDirectory() {
  String result;
  size_t capacity = 128;
  for (;;) {
    result.Resize(capacity);
    if (getcwd(&result[0], capacity))
      break;
    capacity *= 2;
  }
  return result;
}

bool PathExists(const char* path) {
  struct stat st;
  if (TEMP_FAILURE_RETRY(stat(path, &st)) < 0)
    return false;

  return S_ISREG(st.st_mode) || S_ISDIR(st.st_mode);
}

bool PathIsFile(const char* path) {
  struct stat st;
  if (TEMP_FAILURE_RETRY(stat(path, &st)) < 0)
    return false;

  return S_ISREG(st.st_mode);
}

#endif  // !UNIT_TESTS

}  // namespace crazy

// Custom implementation of new and malloc, this prevents dragging
// the libc++ implementation, which drags exception-related machine
// code that is not needed here. This helps reduce the size of the
// final binary considerably.

// IMPORTANT: These symbols are not exported by the crazy linker, thus this
//            does not affect the libraries that it will load, only the
//            linker binary itself!
//
void* operator new(size_t size) {
  void* ptr = ::malloc(size);
  if (ptr != nullptr)
    return ptr;

  // Don't assume it is possible to call any C library function like
  // snprintf() here, since it might allocate heap memory and crash at
  // runtime. Hence our fatal message does not contain the number of
  // bytes requested by the allocation.
  static const char kFatalMessage[] = "Out of memory!";
#ifdef __ANDROID__
  __android_log_write(ANDROID_LOG_FATAL, "crazy_linker", kFatalMessage);
#else
  ::write(2, kFatalMessage, sizeof(kFatalMessage) - 1);
#endif
  _exit(1);

  // NOTE: Adding a 'return nullptr' here will make the compiler error
  // with a message stating that 'operator new(size_t)' is not allowed
  // to return nullptr.
  //
  // Indeed, an new expression like 'new T' shall never return nullptr,
  // and an optimizing compiler will gladly remove any null-checks after
  // them (something the Fuschia team had to learn the hard way when
  // writing their kernel in C++). What is meant here is something like:
  //
  //   Foo* foo = new Foo(10);
  //   if (!foo) {                             <-- entire check and branch
  //      ... Handle out-of-memory condition.  <-- removed by an optimizing
  //   }                                       <-- compiler.
  //
  // C++ provide a non-throwing new expression that can return a nullptr
  // value, but it must be written as 'new (std::nothrow) T' instead of
  // 'new T', and thus nobody uses this. This ends up calling
  // 'operator new(size_t, const std::nothrow_t&)' which is not implemented
  // here.
}

void* operator new[](size_t size) {
  return operator new(size);
}

void operator delete(void* ptr) {
  // The compiler-generated code already checked that |ptr != nullptr|
  // so don't to it a second time.
  ::free(ptr);
}

void operator delete[](void* ptr) {
  ::free(ptr);
}

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_LINUX_SERVICES_SYSCALL_WRAPPERS_H_
#define SANDBOX_LINUX_SERVICES_SYSCALL_WRAPPERS_H_

#include <sys/types.h>

#include "sandbox/sandbox_export.h"

struct sock_fprog;
struct rlimit64;

namespace sandbox {

// Provide direct system call wrappers for a few common system calls.
// These are guaranteed to perform a system call and do not rely on things such
// as caching the current pid (c.f. getpid()) unless otherwise specified.

SANDBOX_EXPORT pid_t sys_getpid(void);

SANDBOX_EXPORT pid_t sys_gettid(void);

SANDBOX_EXPORT long sys_clone(unsigned long flags);

// |regs| is not supported and must be passed as nullptr. |child_stack| must be
// nullptr, since otherwise this function cannot safely return. As a
// consequence, this function does not support CLONE_VM.
SANDBOX_EXPORT long sys_clone(unsigned long flags,
                              decltype(nullptr) child_stack,
                              pid_t* ptid,
                              pid_t* ctid,
                              decltype(nullptr) regs);

// A wrapper for clone with fork-like behavior, meaning that it returns the
// child's pid in the parent and 0 in the child. |flags|, |ptid|, and |ctid| are
// as in the clone system call (the CLONE_VM flag is not supported).
//
// This function uses the libc clone wrapper (which updates libc's pid cache)
// internally, so callers may expect things like getpid() to work correctly
// after in both the child and parent. An exception is when this code is run
// under Valgrind. Valgrind does not support the libc clone wrapper, so the libc
// pid cache may be incorrect after this function is called under Valgrind.
SANDBOX_EXPORT pid_t
ForkWithFlags(unsigned long flags, pid_t* ptid, pid_t* ctid);

SANDBOX_EXPORT void sys_exit_group(int status);

// The official system call takes |args| as void*  (in order to be extensible),
// but add more typing for the cases that are currently used.
SANDBOX_EXPORT int sys_seccomp(unsigned int operation,
                               unsigned int flags,
                               const struct sock_fprog* args);

// Some libcs do not expose a prlimit64 wrapper.
SANDBOX_EXPORT int sys_prlimit64(pid_t pid,
                                 int resource,
                                 const struct rlimit64* new_limit,
                                 struct rlimit64* old_limit);

}  // namespace sandbox

#endif  // SANDBOX_LINUX_SERVICES_SYSCALL_WRAPPERS_H_

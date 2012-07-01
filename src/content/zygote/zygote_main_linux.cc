// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "base/basictypes.h"
#include "base/command_line.h"
#include "base/eintr_wrapper.h"
#include "base/file_path.h"
#include "base/hash_tables.h"
#include "base/linux_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/pickle.h"
#include "base/posix/unix_domain_socket.h"
#include "base/process_util.h"
#include "base/rand_util.h"
#include "base/sys_info.h"
#include "build/build_config.h"
#include "crypto/nss_util.h"
#include "content/common/font_config_ipc_linux.h"
#include "content/common/pepper_plugin_registry.h"
#include "content/common/sandbox_methods_linux.h"
#include "content/common/seccomp_sandbox.h"
#include "content/common/zygote_commands_linux.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"
#include "content/public/common/sandbox_linux.h"
#include "content/public/common/zygote_fork_delegate_linux.h"
#include "content/zygote/zygote_linux.h"
#include "skia/ext/SkFontHost_fontconfig_control.h"
#include "unicode/timezone.h"

#if defined(OS_LINUX)
#include <sys/epoll.h>
#include <sys/prctl.h>
#include <sys/signal.h>
#else
#include <signal.h>
#endif

namespace content {

// See http://code.google.com/p/chromium/wiki/LinuxZygote

static const char kUrandomDevPath[] = "/dev/urandom";

// The SUID sandbox sets this environment variable to a file descriptor
// over which we can signal that we have completed our startup and can be
// chrooted.
static const char kSUIDSandboxVar[] = "SBX_D";

// With SELinux we can carve out a precise sandbox, so we don't have to play
// with intercepting libc calls.
#if !defined(CHROMIUM_SELINUX)

static void ProxyLocaltimeCallToBrowser(time_t input, struct tm* output,
                                        char* timezone_out,
                                        size_t timezone_out_len) {
  Pickle request;
  request.WriteInt(LinuxSandbox::METHOD_LOCALTIME);
  request.WriteString(
      std::string(reinterpret_cast<char*>(&input), sizeof(input)));

  uint8_t reply_buf[512];
  const ssize_t r = UnixDomainSocket::SendRecvMsg(
      Zygote::kMagicSandboxIPCDescriptor, reply_buf, sizeof(reply_buf), NULL,
      request);
  if (r == -1) {
    memset(output, 0, sizeof(struct tm));
    return;
  }

  Pickle reply(reinterpret_cast<char*>(reply_buf), r);
  PickleIterator iter(reply);
  std::string result, timezone;
  if (!reply.ReadString(&iter, &result) ||
      !reply.ReadString(&iter, &timezone) ||
      result.size() != sizeof(struct tm)) {
    memset(output, 0, sizeof(struct tm));
    return;
  }

  memcpy(output, result.data(), sizeof(struct tm));
  if (timezone_out_len) {
    const size_t copy_len = std::min(timezone_out_len - 1, timezone.size());
    memcpy(timezone_out, timezone.data(), copy_len);
    timezone_out[copy_len] = 0;
    output->tm_zone = timezone_out;
  } else {
    output->tm_zone = NULL;
  }
}

static bool g_am_zygote_or_renderer = false;

// Sandbox interception of libc calls.
//
// Because we are running in a sandbox certain libc calls will fail (localtime
// being the motivating example - it needs to read /etc/localtime). We need to
// intercept these calls and proxy them to the browser. However, these calls
// may come from us or from our libraries. In some cases we can't just change
// our code.
//
// It's for these cases that we have the following setup:
//
// We define global functions for those functions which we wish to override.
// Since we will be first in the dynamic resolution order, the dynamic linker
// will point callers to our versions of these functions. However, we have the
// same binary for both the browser and the renderers, which means that our
// overrides will apply in the browser too.
//
// The global |g_am_zygote_or_renderer| is true iff we are in a zygote or
// renderer process. It's set in ZygoteMain and inherited by the renderers when
// they fork. (This means that it'll be incorrect for global constructor
// functions and before ZygoteMain is called - beware).
//
// Our replacement functions can check this global and either proxy
// the call to the browser over the sandbox IPC
// (http://code.google.com/p/chromium/wiki/LinuxSandboxIPC) or they can use
// dlsym with RTLD_NEXT to resolve the symbol, ignoring any symbols in the
// current module.
//
// Other avenues:
//
// Our first attempt involved some assembly to patch the GOT of the current
// module. This worked, but was platform specific and doesn't catch the case
// where a library makes a call rather than current module.
//
// We also considered patching the function in place, but this would again by
// platform specific and the above technique seems to work well enough.

typedef struct tm* (*LocaltimeFunction)(const time_t* timep);
typedef struct tm* (*LocaltimeRFunction)(const time_t* timep,
                                         struct tm* result);
typedef FILE* (*FopenFunction)(const char* path, const char* mode);
typedef int (*XstatFunction)(int version, const char *path, struct stat *buf);
typedef int (*Xstat64Function)(int version, const char *path,
                               struct stat64 *buf);

static pthread_once_t g_libc_localtime_funcs_guard = PTHREAD_ONCE_INIT;
static LocaltimeFunction g_libc_localtime;
static LocaltimeFunction g_libc_localtime64;
static LocaltimeRFunction g_libc_localtime_r;
static LocaltimeRFunction g_libc_localtime64_r;

// http://crbug.com/123263, see below.
#if !defined(ADDRESS_SANITIZER)
static pthread_once_t g_libc_file_io_funcs_guard = PTHREAD_ONCE_INIT;
static FopenFunction g_libc_fopen;
static FopenFunction g_libc_fopen64;
static XstatFunction g_libc_xstat;
static Xstat64Function g_libc_xstat64;
#endif

static void InitLibcLocaltimeFunctions() {
  g_libc_localtime = reinterpret_cast<LocaltimeFunction>(
      dlsym(RTLD_NEXT, "localtime"));
  g_libc_localtime64 = reinterpret_cast<LocaltimeFunction>(
      dlsym(RTLD_NEXT, "localtime64"));
  g_libc_localtime_r = reinterpret_cast<LocaltimeRFunction>(
      dlsym(RTLD_NEXT, "localtime_r"));
  g_libc_localtime64_r = reinterpret_cast<LocaltimeRFunction>(
      dlsym(RTLD_NEXT, "localtime64_r"));

  if (!g_libc_localtime || !g_libc_localtime_r) {
    // http://code.google.com/p/chromium/issues/detail?id=16800
    //
    // Nvidia's libGL.so overrides dlsym for an unknown reason and replaces
    // it with a version which doesn't work. In this case we'll get a NULL
    // result. There's not a lot we can do at this point, so we just bodge it!
    LOG(ERROR) << "Your system is broken: dlsym doesn't work! This has been "
                  "reported to be caused by Nvidia's libGL. You should expect"
                  " time related functions to misbehave. "
                  "http://code.google.com/p/chromium/issues/detail?id=16800";
  }

  if (!g_libc_localtime)
    g_libc_localtime = gmtime;
  if (!g_libc_localtime64)
    g_libc_localtime64 = g_libc_localtime;
  if (!g_libc_localtime_r)
    g_libc_localtime_r = gmtime_r;
  if (!g_libc_localtime64_r)
    g_libc_localtime64_r = g_libc_localtime_r;
}

// Define localtime_override() function with asm name "localtime", so that all
// references to localtime() will resolve to this function. Notice that we need
// to set visibility attribute to "default" to export the symbol, as it is set
// to "hidden" by default in chrome per build/common.gypi.
__attribute__ ((__visibility__("default")))
struct tm* localtime_override(const time_t* timep) __asm__ ("localtime");

__attribute__ ((__visibility__("default")))
struct tm* localtime_override(const time_t* timep) {
  if (g_am_zygote_or_renderer) {
    static struct tm time_struct;
    static char timezone_string[64];
    ProxyLocaltimeCallToBrowser(*timep, &time_struct, timezone_string,
                                sizeof(timezone_string));
    return &time_struct;
  } else {
    CHECK_EQ(0, pthread_once(&g_libc_localtime_funcs_guard,
                             InitLibcLocaltimeFunctions));
    return g_libc_localtime(timep);
  }
}

// Use same trick to override localtime64(), localtime_r() and localtime64_r().
__attribute__ ((__visibility__("default")))
struct tm* localtime64_override(const time_t* timep) __asm__ ("localtime64");

__attribute__ ((__visibility__("default")))
struct tm* localtime64_override(const time_t* timep) {
  if (g_am_zygote_or_renderer) {
    static struct tm time_struct;
    static char timezone_string[64];
    ProxyLocaltimeCallToBrowser(*timep, &time_struct, timezone_string,
                                sizeof(timezone_string));
    return &time_struct;
  } else {
    CHECK_EQ(0, pthread_once(&g_libc_localtime_funcs_guard,
                             InitLibcLocaltimeFunctions));
    return g_libc_localtime64(timep);
  }
}

__attribute__ ((__visibility__("default")))
struct tm* localtime_r_override(const time_t* timep,
                                struct tm* result) __asm__ ("localtime_r");

__attribute__ ((__visibility__("default")))
struct tm* localtime_r_override(const time_t* timep, struct tm* result) {
  if (g_am_zygote_or_renderer) {
    ProxyLocaltimeCallToBrowser(*timep, result, NULL, 0);
    return result;
  } else {
    CHECK_EQ(0, pthread_once(&g_libc_localtime_funcs_guard,
                             InitLibcLocaltimeFunctions));
    return g_libc_localtime_r(timep, result);
  }
}

__attribute__ ((__visibility__("default")))
struct tm* localtime64_r_override(const time_t* timep,
                                  struct tm* result) __asm__ ("localtime64_r");

__attribute__ ((__visibility__("default")))
struct tm* localtime64_r_override(const time_t* timep, struct tm* result) {
  if (g_am_zygote_or_renderer) {
    ProxyLocaltimeCallToBrowser(*timep, result, NULL, 0);
    return result;
  } else {
    CHECK_EQ(0, pthread_once(&g_libc_localtime_funcs_guard,
                             InitLibcLocaltimeFunctions));
    return g_libc_localtime64_r(timep, result);
  }
}

// TODO(sergeyu): Currently this code doesn't work properly under ASAN
// - it crashes content_unittests. Make sure it works properly and
// enable it here. http://crbug.com/123263
#if !defined(ADDRESS_SANITIZER)

static void InitLibcFileIOFunctions() {
  g_libc_fopen = reinterpret_cast<FopenFunction>(
      dlsym(RTLD_NEXT, "fopen"));
  g_libc_fopen64 = reinterpret_cast<FopenFunction>(
      dlsym(RTLD_NEXT, "fopen64"));

  if (!g_libc_fopen) {
    LOG(FATAL) << "Failed to get fopen() from libc.";
  } else if (!g_libc_fopen64) {
#if !defined(OS_OPENBSD) && !defined(OS_FREEBSD)
    LOG(WARNING) << "Failed to get fopen64() from libc. Using fopen() instead.";
#endif  // !defined(OS_OPENBSD) && !defined(OS_FREEBSD)
    g_libc_fopen64 = g_libc_fopen;
  }

  // TODO(sergeyu): This works only on systems with glibc. Fix it to
  // work properly on other systems if necessary.
  g_libc_xstat = reinterpret_cast<XstatFunction>(
      dlsym(RTLD_NEXT, "__xstat"));
  g_libc_xstat64 = reinterpret_cast<Xstat64Function>(
      dlsym(RTLD_NEXT, "__xstat64"));

  if (!g_libc_xstat) {
    LOG(FATAL) << "Failed to get __xstat() from libc.";
  }
  if (!g_libc_xstat64) {
    LOG(WARNING) << "Failed to get __xstat64() from libc.";
  }
}

// fopen() and fopen64() are intercepted here so that NSS can open
// /dev/urandom to seed its random number generator. NSS is used by
// remoting in the sendbox.

// fopen() call may be redirected to fopen64() in stdio.h using
// __REDIRECT(), which sets asm name for fopen() to "fopen64". This
// means that we cannot override fopen() directly here. Instead the
// the code below defines fopen_override() function with asm name
// "fopen", so that all references to fopen() will resolve to this
// function.
__attribute__ ((__visibility__("default")))
FILE* fopen_override(const char* path, const char* mode)  __asm__ ("fopen");

__attribute__ ((__visibility__("default")))
FILE* fopen_override(const char* path, const char* mode) {
  if (g_am_zygote_or_renderer && strcmp(path, kUrandomDevPath) == 0) {
    int fd = HANDLE_EINTR(dup(base::GetUrandomFD()));
    if (fd < 0) {
      PLOG(ERROR) << "dup() failed.";
      return NULL;
    }
    return fdopen(fd, mode);
  } else {
    CHECK_EQ(0, pthread_once(&g_libc_file_io_funcs_guard,
                             InitLibcFileIOFunctions));
    return g_libc_fopen(path, mode);
  }
}

__attribute__ ((__visibility__("default")))
FILE* fopen64(const char* path, const char* mode) {
  if (g_am_zygote_or_renderer && strcmp(path, kUrandomDevPath) == 0) {
    int fd = HANDLE_EINTR(dup(base::GetUrandomFD()));
    if (fd < 0) {
      PLOG(ERROR) << "dup() failed.";
      return NULL;
    }
    return fdopen(fd, mode);
  } else {
    CHECK_EQ(0, pthread_once(&g_libc_file_io_funcs_guard,
                             InitLibcFileIOFunctions));
    return g_libc_fopen64(path, mode);
  }
}

// stat() is subject to the same problem as fopen(), so we have to use
// the same trick to override it.
__attribute__ ((__visibility__("default")))
int xstat_override(int version,
                   const char *path,
                   struct stat *buf)  __asm__ ("__xstat");

__attribute__ ((__visibility__("default")))
int xstat_override(int version, const char *path, struct stat *buf) {
  if (g_am_zygote_or_renderer && strcmp(path, kUrandomDevPath) == 0) {
    int result = __fxstat(version, base::GetUrandomFD(), buf);
    return result;
  } else {
    CHECK_EQ(0, pthread_once(&g_libc_file_io_funcs_guard,
                             InitLibcFileIOFunctions));
    return g_libc_xstat(version, path, buf);
  }
}

__attribute__ ((__visibility__("default")))
int xstat64_override(int version,
                     const char *path,
                     struct stat64 *buf)  __asm__ ("__xstat64");

__attribute__ ((__visibility__("default")))
int xstat64_override(int version, const char *path, struct stat64 *buf) {
  if (g_am_zygote_or_renderer && strcmp(path, kUrandomDevPath) == 0) {
    int result = __fxstat64(version, base::GetUrandomFD(), buf);
    return result;
  } else {
    CHECK_EQ(0, pthread_once(&g_libc_file_io_funcs_guard,
                             InitLibcFileIOFunctions));
    CHECK(g_libc_xstat64);
    return g_libc_xstat64(version, path, buf);
  }
}

#endif  // !ADDRESS_SANITIZER

#endif  // !CHROMIUM_SELINUX

// This function triggers the static and lazy construction of objects that need
// to be created before imposing the sandbox.
static void PreSandboxInit() {
  base::RandUint64();

  base::SysInfo::MaxSharedMemorySize();

  // ICU DateFormat class (used in base/time_format.cc) needs to get the
  // Olson timezone ID by accessing the zoneinfo files on disk. After
  // TimeZone::createDefault is called once here, the timezone ID is
  // cached and there's no more need to access the file system.
  scoped_ptr<icu::TimeZone> zone(icu::TimeZone::createDefault());

#if defined(USE_NSS)
  // NSS libraries are loaded before sandbox is activated. This is to allow
  // successful initialization of NSS which tries to load extra library files.
  crypto::LoadNSSLibraries();
#elif defined(USE_OPENSSL)
  // OpenSSL is intentionally not supported in the sandboxed processes, see
  // http://crbug.com/99163. If that ever changes we'll likely need to init
  // OpenSSL here (at least, load the library and error strings).
#else
  // It's possible that another hypothetical crypto stack would not require
  // pre-sandbox init, but more likely this is just a build configuration error.
  #error Which SSL library are you using?
#endif

  // Ensure access to the Pepper plugins before the sandbox is turned on.
  PepperPluginRegistry::PreloadModules();
}

#if !defined(CHROMIUM_SELINUX)
// Do nothing here
static void SIGCHLDHandler(int signal) {
}

// The current process will become a process reaper like init.
// We fork a child that will continue normally, when it dies, we can safely
// exit.
// We need to be careful we close the magic kZygoteIdFd properly in the parent
// before this function returns.
static bool CreateInitProcessReaper() {
  int sync_fds[2];
  // We want to use send, so we can't use a pipe
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sync_fds)) {
    LOG(ERROR) << "Failed to create socketpair";
    return false;
  }

  // We use normal fork, not the ForkDelegate in this case since we are not a
  // true Zygote yet.
  pid_t child_pid = fork();
  if (child_pid == -1) {
    (void) HANDLE_EINTR(close(sync_fds[0]));
    (void) HANDLE_EINTR(close(sync_fds[1]));
    return false;
  }
  if (child_pid) {
    // We are the parent, assuming the role of an init process.
    // The disposition for SIGCHLD cannot be SIG_IGN or wait() will only return
    // once all of our childs are dead. Since we're init we need to reap childs
    // as they come.
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = &SIGCHLDHandler;
    CHECK(sigaction(SIGCHLD, &action, NULL) == 0);

    (void) HANDLE_EINTR(close(sync_fds[0]));
    shutdown(sync_fds[1], SHUT_RD);
    // This "magic" socket must only appear in one process.
    (void) HANDLE_EINTR(close(content::kZygoteIdFd));
    // Tell the child to continue
    CHECK(HANDLE_EINTR(send(sync_fds[1], "C", 1, MSG_NOSIGNAL)) == 1);
    (void) HANDLE_EINTR(close(sync_fds[1]));

    for (;;) {
      // Loop until we have reaped our one natural child
      siginfo_t reaped_child_info;
      int wait_ret =
        HANDLE_EINTR(waitid(P_ALL, 0, &reaped_child_info, WEXITED));
      if (wait_ret)
        _exit(1);
      if (reaped_child_info.si_pid == child_pid) {
        int exit_code = 0;
        // We're done waiting
        if (reaped_child_info.si_code == CLD_EXITED) {
          exit_code = reaped_child_info.si_status;
        }
        // Exit with the same exit code as our parent. This is most likely
        // useless. _exit with 0 if we got signaled.
        _exit(exit_code);
      }
    }
  } else {
    // The child needs to wait for the parent to close kZygoteIdFd to avoid a
    // race condition
    (void) HANDLE_EINTR(close(sync_fds[1]));
    shutdown(sync_fds[0], SHUT_WR);
    char should_continue;
    int read_ret = HANDLE_EINTR(read(sync_fds[0], &should_continue, 1));
    (void) HANDLE_EINTR(close(sync_fds[0]));
    if (read_ret == 1)
      return true;
    else
      return false;
  }
}

// This will set the *using_suid_sandbox variable to true if the SUID sandbox
// is enabled. This does not necessarily exclude other types of sandboxing.
static bool EnterSandbox(bool* using_suid_sandbox, bool* has_started_new_init) {
  *using_suid_sandbox = false;
  *has_started_new_init = false;

  PreSandboxInit();
  SkiaFontConfigSetImplementation(
      new FontConfigIPC(Zygote::kMagicSandboxIPCDescriptor));

  const char* const sandbox_fd_string = getenv(kSUIDSandboxVar);
  if (sandbox_fd_string) {
    char* endptr;
    // Use the SUID sandbox.  This still allows the seccomp sandbox to
    // be enabled by the process later.
    *using_suid_sandbox = true;

    // Check if the SUID sandbox provides the correct API version.
    const char* const sandbox_api_string =
                        getenv(base::kSandboxEnvironmentApiProvides);
    // Assume API version 0 if no environment was found
    long sandbox_api_num = 0;
    if (sandbox_api_string) {
      errno = 0;
      sandbox_api_num = strtol(sandbox_api_string, &endptr, 10);
      if (errno || *endptr) {
        return false;
      }
    }

    if (sandbox_api_num != base::kSUIDSandboxApiNumber) {
      LOG(WARNING) << "You are using a wrong version of the setuid binary!\n"
       "Please read "
       "https://code.google.com/p/chromium/wiki/LinuxSUIDSandboxDevelopment."
       "\n\n";
    }

    // Get the file descriptor to signal the chroot helper.
    errno = 0;
    const long fd_long = strtol(sandbox_fd_string, &endptr, 10);
    if (errno || !*sandbox_fd_string || *endptr || fd_long < 0 ||
        fd_long > INT_MAX) {
      return false;
    }
    const int fd = fd_long;

    static const char kMsgChrootMe = 'C';
    static const char kMsgChrootSuccessful = 'O';

    if (HANDLE_EINTR(write(fd, &kMsgChrootMe, 1)) != 1) {
      LOG(ERROR) << "Failed to write to chroot pipe: " << errno;
      return false;
    }

    // We need to reap the chroot helper process in any event:
    wait(NULL);

    char reply;
    if (HANDLE_EINTR(read(fd, &reply, 1)) != 1) {
      LOG(ERROR) << "Failed to read from chroot pipe: " << errno;
      return false;
    }

    if (reply != kMsgChrootSuccessful) {
      LOG(ERROR) << "Error code reply from chroot helper";
      return false;
    }

    if (getpid() == 1) {
      // The setuid sandbox has created a new PID namespace and we need
      // to assume the role of init.
      if (!CreateInitProcessReaper()) {
        LOG(ERROR) << "Error creating an init process to reap zombies";
        return false;
      }
      *has_started_new_init = true;
    }

#if !defined(OS_OPENBSD)
    // Previously, we required that the binary be non-readable. This causes the
    // kernel to mark the process as non-dumpable at startup. The thinking was
    // that, although we were putting the renderers into a PID namespace (with
    // the SUID sandbox), they would nonetheless be in the /same/ PID
    // namespace. So they could ptrace each other unless they were non-dumpable.
    //
    // If the binary was readable, then there would be a window between process
    // startup and the point where we set the non-dumpable flag in which a
    // compromised renderer could ptrace attach.
    //
    // However, now that we have a zygote model, only the (trusted) zygote
    // exists at this point and we can set the non-dumpable flag which is
    // inherited by all our renderer children.
    //
    // Note: a non-dumpable process can't be debugged. To debug sandbox-related
    // issues, one can specify --allow-sandbox-debugging to let the process be
    // dumpable.
    const CommandLine& command_line = *CommandLine::ForCurrentProcess();
    if (!command_line.HasSwitch(switches::kAllowSandboxDebugging)) {
      prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
      if (prctl(PR_GET_DUMPABLE, 0, 0, 0, 0)) {
        LOG(ERROR) << "Failed to set non-dumpable flag";
        return false;
      }
    }
#endif
  }

  return true;
}
#else  // CHROMIUM_SELINUX

static bool EnterSandbox(bool* using_suid_sandbox, bool* has_started_new_init) {
  *using_suid_sandbox = false;
  *has_started_new_init = false;

  PreSandboxInit();
  SkiaFontConfigSetImplementation(
      new FontConfigIPC(Zygote::kMagicSandboxIPCDescriptor));
  return true;
}

#endif  // CHROMIUM_SELINUX

bool ZygoteMain(const MainFunctionParams& params,
                ZygoteForkDelegate* forkdelegate) {
#if !defined(CHROMIUM_SELINUX)
  g_am_zygote_or_renderer = true;
#endif

  int proc_fd_for_seccomp = -1;
#if defined(SECCOMP_SANDBOX)
  if (SeccompSandboxEnabled()) {
    // The seccomp sandbox needs access to files in /proc, which might be denied
    // after one of the other sandboxes have been started. So, obtain a suitable
    // file handle in advance.
    proc_fd_for_seccomp = open("/proc", O_DIRECTORY | O_RDONLY);
    if (proc_fd_for_seccomp < 0) {
      LOG(ERROR) << "WARNING! Cannot access \"/proc\". Disabling seccomp "
          "sandboxing.";
    }
  }
#endif  // SECCOMP_SANDBOX

  if (forkdelegate != NULL) {
    VLOG(1) << "ZygoteMain: initializing fork delegate";
    forkdelegate->Init(getenv(kSUIDSandboxVar) != NULL,
                       Zygote::kBrowserDescriptor,
                       Zygote::kMagicSandboxIPCDescriptor);
  } else {
    VLOG(1) << "ZygoteMain: fork delegate is NULL";
  }

  // Turn on the SELinux or SUID sandbox.
  bool using_suid_sandbox = false;
  bool has_started_new_init = false;
  if (!EnterSandbox(&using_suid_sandbox, &has_started_new_init)) {
    LOG(FATAL) << "Failed to enter sandbox. Fail safe abort. (errno: "
               << errno << ")";
    return false;
  }

  int sandbox_flags = 0;
  if (using_suid_sandbox) {
    sandbox_flags |= kSandboxLinuxSUID;
    if (getenv("SBX_PID_NS"))
      sandbox_flags |= kSandboxLinuxPIDNS;
    if (getenv("SBX_NET_NS"))
      sandbox_flags |= kSandboxLinuxNetNS;
  }

  if ((sandbox_flags & kSandboxLinuxPIDNS) && !has_started_new_init) {
    LOG(ERROR) << "The SUID sandbox created a new PID namespace but Zygote "
                  "is not the init process. Please, make sure the SUID "
                  "binary is up to date.";
  }

#if defined(SECCOMP_SANDBOX)
  // The seccomp sandbox will be turned on when the renderers start. But we can
  // already check if sufficient support is available so that we only need to
  // print one error message for the entire browser session.
  if (proc_fd_for_seccomp >= 0 && SeccompSandboxEnabled()) {
    if (!SupportsSeccompSandbox(proc_fd_for_seccomp)) {
      // There are a good number of users who cannot use the seccomp sandbox
      // (e.g. because their distribution does not enable seccomp mode by
      // default). While we would prefer to deny execution in this case, it
      // seems more realistic to continue in degraded mode.
      LOG(ERROR) << "WARNING! This machine lacks support needed for the "
                    "Seccomp sandbox. Running renderers with Seccomp "
                    "sandboxing disabled.";
      close(proc_fd_for_seccomp);
      proc_fd_for_seccomp = -1;
    } else {
      VLOG(1) << "Enabling experimental Seccomp sandbox.";
      sandbox_flags |= kSandboxLinuxSeccomp;
    }
  }
#endif  // SECCOMP_SANDBOX

  Zygote zygote(sandbox_flags, forkdelegate, proc_fd_for_seccomp);
  // This function call can return multiple times, once per fork().
  return zygote.ProcessRequests();
}

}  // namespace content

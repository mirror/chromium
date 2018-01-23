// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <AvailabilityMacros.h>
#include <copyfile.h>
#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace {

class ScopedUnlink {
 public:
  explicit ScopedUnlink(const char* path) : path_(path) {}

  ~ScopedUnlink() {
    if (path_ && unlink(path_) != 0) {
      warn("warning: unlink(\"%s\")", path_);
    }
  }

  void Reset() { path_ = nullptr; }

 private:
  const char* path_;
};

int RenameXNp(const char* from, const char* to, unsigned int flags) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability"
#if !defined(MAC_OS_X_VERSION_10_12) || \
    MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_12
  if (!renamex_np) {
    errno = ENOTSUP;
    return -1;
  }
#endif

  return renamex_np(from, to, flags);
#pragma clang diagnostic pop
}

bool ReplaceFile(char* source_path, char* target_path) {
  char tmp_path[PATH_MAX];
  snprintf(tmp_path, sizeof(tmp_path), "%s/.%s.XXXXXX", dirname(target_path),
           basename(target_path));
  int tmp_fd = mkstemp(tmp_path);
  if (tmp_fd < 0) {
    warn("mkstemp(\"%s\")", tmp_path);
    return false;
  }

  ScopedUnlink unlink_tmp_path(tmp_path);

  if (close(tmp_fd) != 0) {
    warn("warning: close(\"%s\")", tmp_path);
  }

  if (copyfile(source_path, tmp_path, nullptr, COPYFILE_ALL) != 0) {
    warn("copyfile(\"%s\", \"%s\", COPYFILE_ALL) (1)", source_path, tmp_path);
    return false;
  }

  // Try the method that works on APFS first.
  if (RenameXNp(tmp_path, target_path, RENAME_SWAP) == 0) {
    if (copyfile(source_path, tmp_path, nullptr, COPYFILE_ALL) != 0) {
      warn("copyfile(\"%s\", \"%s\", COPYFILE_ALL) (2)", source_path, tmp_path);
      return false;
    }
    if (RenameXNp(tmp_path, target_path, RENAME_SWAP) != 0) {
      warn("renamex_np(\"%s\", \"%s\", RENAME_SWAP) (2)", tmp_path,
           target_path);
      return false;
    }
  } else if (errno == ENOTSUP) {
    // Try the method that works on HFS+.
    if (exchangedata(tmp_path, target_path, 0) != 0) {
      if (errno == ENOTSUP) {
        // Neither APFS nor HFS+. Fall back to a straight “rename”.
        if (rename(tmp_path, target_path) != 0) {
          warn("rename(\"%s\", \"%s\")", tmp_path, target_path);
          return false;
        }
        unlink_tmp_path.Reset();
        return true;
      }
      warn("exchangedata(\"%s\", \"%s\")", tmp_path, target_path);
      return false;
    }

    if (copyfile(tmp_path, target_path, nullptr, COPYFILE_METADATA) != 0) {
      warn("warning: copyfile(\"%s\", \"%s\", COPYFILE_METADATA)", tmp_path,
           target_path);
    }
  } else {
    warn("renamex_np(\"%s\", \"%s\", RENAME_SWAP) (1)", tmp_path, target_path);
    return false;
  }

  return true;
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s source target\n", basename(argv[0]));
    return EXIT_FAILURE;
  }

  return ReplaceFile(argv[1], argv[2]) ? EXIT_SUCCESS : EXIT_FAILURE;
}

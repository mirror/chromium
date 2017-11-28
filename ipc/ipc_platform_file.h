// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IPC_IPC_PLATFORM_FILE_H_
#define IPC_IPC_PLATFORM_FILE_H_

#include "base/files/file.h"
#include "base/process/process.h"
#include "build/build_config.h"
#include "ipc/ipc_message_support_export.h"

#if defined(OS_POSIX)
#include "base/file_descriptor_posix.h"
#endif

namespace IPC {

class IPC_MESSAGE_SUPPORT_EXPORT PlatformFileForTransit {
 public:
#if defined(OS_WIN)
  using FileType = HANDLE;
#else
  using FileType = base::FileDescriptor;
#endif

  // Creates an invalid platform file.
  PlatformFileForTransit();

  // Creates a platform file that takes unofficial ownership of |handle|. Note
  // that ownership is not handled by a Scoped* class. On Windows, when this
  // class is used as an input to an IPC message, the IPC subsystem will take
  // ownership of |file|. On other platforms, it respects whether or not |file|
  // owns its own fd. When this class is used as the output from an IPC message,
  // the receiver is expected to take ownership of |file|.
  //
  // |is_async| must be true if the file is open for async IO.
  PlatformFileForTransit(FileType file, bool is_async);

  // Comparison operators.
  bool operator==(const PlatformFileForTransit& platform_file) const;
  bool operator!=(const PlatformFileForTransit& platform_file) const;

  FileType GetFile() const;
  bool IsValid() const;
  bool IsAsync() const;

 private:
  FileType file_;
  bool is_async_;
};

inline PlatformFileForTransit InvalidPlatformFileForTransit() {
  return PlatformFileForTransit();
}

inline base::PlatformFile PlatformFileForTransitToPlatformFile(
    const PlatformFileForTransit& transit) {
#if defined(OS_WIN)
  return transit.GetFile();
#elif defined(OS_POSIX)
  return transit.GetFile().fd;
#endif
}

inline base::File PlatformFileForTransitToFile(
    const PlatformFileForTransit& transit) {
  base::PlatformFile file;
#if defined(OS_WIN)
  file = transit.GetFile();
#elif defined(OS_POSIX)
  file = transit.GetFile().fd;
#endif
  if (!transit.IsAsync()) {
    return base::File(file);
  } else {
    return base::File::CreateForAsyncHandle(file);
  }
}

// Creates a new handle that can be passed through IPC. The result must be
// passed to the IPC layer as part of a message, or else it will leak.
IPC_MESSAGE_SUPPORT_EXPORT PlatformFileForTransit
GetPlatformFileForTransit(base::PlatformFile file,
                          bool close_source_handle,
                          bool is_async);

// Creates a new handle that can be passed through IPC. The result must be
// passed to the IPC layer as part of a message, or else it will leak.
// Note that this function takes ownership of |file|.
IPC_MESSAGE_SUPPORT_EXPORT PlatformFileForTransit
TakePlatformFileForTransit(base::File file);

// Creates a new handle that can be passed through IPC. The result must be
// passed to the IPC layer as part of a message, or else it will leak.
// Note that this function does not take ownership of |file|.
IPC_MESSAGE_SUPPORT_EXPORT PlatformFileForTransit
DuplicatePlatformFileForTransit(const base::File& file);

}  // namespace IPC

#endif  // IPC_IPC_PLATFORM_FILE_H_

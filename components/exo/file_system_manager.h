// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_FILE_SYSTEM_MANAGER_H_
#define COMPONENTS_EXO_FILE_SYSTEM_MANAGER_H_

#include <string>

class GURL;

namespace base {
class FilePath;
}

namespace exo {

enum class ContainerType { kArc };

class FileSystemManager {
 public:
  // Convert natife file path to URL which can be used in container.  We don't
  // expose enter file system to a container directly.  Instead we mount
  // specific directory in the containers' namespace.  Thus we need to convert
  // native path to file URL which points mount point in containers.  The
  // conversion should be container specific, now we only have ARC container
  // though.
  virtual bool ConvertPathToUrl(ContainerType container_type,
                                const base::FilePath& path,
                                GURL* out) = 0;

 protected:
  virtual ~FileSystemManager() {}
};

}  // namespace exo

#endif  // COMPONENTS_EXO_FILE_SYSTEM_MANAGER_H_

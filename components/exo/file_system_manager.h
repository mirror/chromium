// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_FILE_SYSTEM_MANAGER_H_
#define COMPONENTS_EXO_FILE_SYSTEM_MANAGER_H_

#include <string>

namespace exo {

enum class ContainerType { kArc };

class FileSystemManager {
 public:
  // Convert natife file path to URL which can be used in container.
  virtual bool ConvertPathToUrl(ContainerType container_type,
                                const base::Path& path,
                                GURL* out) = 0;

 protected:
  virtual ~FileSystemManager() {}
};

}  // namespace exo

#endif  // COMPONENTS_EXO_FILE_SYSTEM_MANAGER_H_

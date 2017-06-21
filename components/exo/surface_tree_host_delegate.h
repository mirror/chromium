// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_SURFACE_TREE_HOST_DELEGATE_H_
#define COMPONENTS_EXO_SURFACE_TREE_HOST_DELEGATE_H_

#include "base/macros.h"

namespace exo {

class Surface;

class SurfaceTreeHostDelegate {
 public:
  SurfaceTreeHostDelegate() = default;
  virtual ~SurfaceTreeHostDelegate() = default;

  virtual void OnSurfaceTreeCommit() = 0;
  virtual void OnSurfaceDetached(Surface* surface) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(SurfaceTreeHostDelegate);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_SURFACE_TREE_HOST_DELEGATE_H_

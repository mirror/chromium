// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_POINTER_OBSERVER_H_
#define COMPONENTS_EXO_POINTER_OBSERVER_H_

namespace exo {
class Pointer;

// Observers to the Pointer are notified when the Pointer destructs.
class PointerObserver {
 public:
  virtual ~PointerObserver() {}

  // Called at the top of the pointer's destructor, to give observers a change
  // to remove themselves.
  virtual void OnPointerDestroying(Pointer* pointer) = 0;
};

}  // namespace exo

#endif  // COMPONENTS_EXO_POINTER_OBSERVER_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_SEAT_OBSERVER_H_
#define COMPONENTS_EXO_SEAT_OBSERVER_H_

#include <string>

namespace exo {

class Surface;

// Handles events on seat in context-specific ways.
class SeatObserver {
 public:
  virtual void OnSurfaceFocused(Surface* surface) = 0;

 protected:
  virtual ~SeatObserver() {}
};

}  // namespace exo

#endif  // COMPONENTS_EXO_SEAT_OBSERVER_H_

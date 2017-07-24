// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_DATABINDING_BINDING_H_
#define CHROME_BROWSER_VR_DATABINDING_BINDING_H_

namespace vr {

class Binding {
 public:
  Binding() {}
  virtual ~Binding() {}

  // This function updates the binding. The exact behavior depends on the
  // subclass. Please see comments on the overridden functions for details.
  virtual void Update() = 0;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_DATABINDING_BINDING_H_

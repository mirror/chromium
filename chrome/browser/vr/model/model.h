// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_MODEL_H_
#define CHROME_BROWSER_VR_MODEL_MODEL_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace vr {

// This represents our browser state. UI elements will be bound to these values.
class Model {
 public:
  Model();
  ~Model();

  bool loading() const { return loading_; }
  void set_loading(bool loading) { loading_ = loading; }

  float load_progress() const { return load_progress_; }
  void set_load_progress(float load_progress) {
    load_progress_ = load_progress;
  }

  base::WeakPtr<Model> GetWeakPtr();

 private:
  bool loading_ = false;
  float load_progress_ = 0.0f;

  base::WeakPtrFactory<Model> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(Model);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_MODEL_H_

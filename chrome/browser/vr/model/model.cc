// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/model/model.h"

namespace vr {

Model::Model() : weak_ptr_factory_(this) {}
Model::~Model() = default;

base::WeakPtr<Model> Model::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace vr

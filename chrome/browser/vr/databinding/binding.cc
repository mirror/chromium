// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/databinding/binding.h"

namespace vr {

Binding::Binding() : weak_ptr_factory_(this) {}
Binding::~Binding() {}

base::WeakPtr<Binding> Binding::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace vr

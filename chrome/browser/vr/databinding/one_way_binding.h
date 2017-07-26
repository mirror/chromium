// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_DATABINDING_ONE_WAY_BINDING_H_
#define CHROME_BROWSER_VR_DATABINDING_ONE_WAY_BINDING_H_

#include "base/bind.h"
#include "base/macros.h"
#include "base/optional.h"
#include "chrome/browser/vr/databinding/binding.h"

namespace vr {

template <typename T>
class OneWayBinding : public Binding {
 public:
  OneWayBinding(const base::Callback<T()>& getter,
                const base::Callback<void(const T&)>& setter)
      : getter_(getter), setter_(setter) {}
  ~OneWayBinding() override {}

  // This function will check if the getter is producing a different value than
  // when it was last polled. If so, it will pass that value to the provided
  // setter.
  void Update() override {
    T current_value = getter_.Run();
    if (!last_value_ || current_value != last_value_.value()) {
      last_value_ = current_value;
      setter_.Run(current_value);
    }
  }

 private:
  base::Callback<T()> getter_;
  base::Callback<void(const T&)> setter_;
  base::Optional<T> last_value_;

  DISALLOW_COPY_AND_ASSIGN(OneWayBinding);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_DATABINDING_ONE_WAY_BINDING_H_

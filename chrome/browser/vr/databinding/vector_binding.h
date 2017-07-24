// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_DATABINDING_VECTOR_BINDING_H_
#define CHROME_BROWSER_VR_DATABINDING_VECTOR_BINDING_H_

#include <vector>

#include "base/bind.h"
#include "base/macros.h"
#include "base/optional.h"
#include "chrome/browser/vr/databinding/binding.h"

namespace vr {

// This class manages databinding for a vector of objects.
template <typename M, typename B>
class VectorBinding : public Binding {
 public:
  typedef base::Callback<std::unique_ptr<B>(std::vector<M>*, size_t)>
      ModelAddedCallback;
  typedef base::Callback<void(B*)> ModelRemovedCallback;

  VectorBinding(std::vector<M>* models,
                const ModelAddedCallback& added_callback,
                const ModelRemovedCallback& removed_callback)
      : models_(models),
        added_callback_(added_callback),
        removed_callback_(removed_callback) {}

  ~VectorBinding() {}

  // This function will check if the getter is producing a different value than
  // when it was last polled. If so, it will pass that value to the provided
  // setter.
  void Update() override {
    size_t current_size = models_->size();
    if (!last_size_ || current_size != last_size_.value()) {
      size_t last_size = last_size_ ? last_size_.value() : 0lu;
      for (size_t i = current_size; i < last_size; ++i) {
        removed_callback_.Run(bindings_[i].get());
      }
      bindings_.resize(current_size);
      for (size_t i = last_size; i < current_size; ++i) {
        bindings_[i] = std::move(added_callback_.Run(models_, i));
      }
      last_size_ = current_size;
    }
    for (auto& binding : bindings_) {
      binding->Update();
    }
  }

 private:
  std::vector<M>* models_ = nullptr;
  std::vector<std::unique_ptr<B>> bindings_;
  base::Optional<size_t> last_size_;
  ModelAddedCallback added_callback_;
  ModelRemovedCallback removed_callback_;

  DISALLOW_COPY_AND_ASSIGN(VectorBinding);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_DATABINDING_VECTOR_BINDING_H_

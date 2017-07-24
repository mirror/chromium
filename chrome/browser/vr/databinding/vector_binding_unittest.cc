// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/databinding/vector_binding.h"

#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "chrome/browser/vr/databinding/one_way_binding.h"
#include "chrome/browser/vr/databinding/vector_element_binding.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace vr {

namespace {

struct View;

class ViewRegistry {
 public:
  void AddView(std::unique_ptr<View> view) {
    views_.push_back(std::move(view));
  }
  void RemoveView(View* view) {
    base::EraseIf(views_, [view](const std::unique_ptr<View>& v) {
      return v.get() == view;
    });
  }

  const std::vector<std::unique_ptr<View>>& views() { return views_; }
private:
  std::vector<std::unique_ptr<View>> views_;
};

struct View {
  int value;
};

struct Model {
  int value;
};

typedef VectorElementBinding<Model, View> ElemBinding;
typedef VectorBinding<Model, ElemBinding> TestVectorBinding;

int GetValue(std::vector<Model>* models, size_t i) {
  return (*models)[i].value;
}

void SetValue(View* view, const int& value) {
  view->value = value;
}

std::unique_ptr<ElemBinding> OnModelAdded(ViewRegistry* registry,
                                          std::vector<Model>* models,
                                          size_t i) {
  std::unique_ptr<ElemBinding> elem_binding =
      base::MakeUnique<ElemBinding>(models, i);
  std::unique_ptr<View> view = base::MakeUnique<View>();
  elem_binding->set_view(view.get());

  std::unique_ptr<Binding> model_view_binding(new OneWayBinding<int>(
      base::Bind(&GetValue, base::Unretained(models), i),
      base::Bind(&SetValue, base::Unretained(view.get()))));

  elem_binding->bindings().push_back(std::move(model_view_binding));

  registry->AddView(std::move(view));
  return elem_binding;
}

void OnModelRemoved(ViewRegistry* registry, ElemBinding* binding) {
  registry->RemoveView(binding->view());
}

}  // namespace

TEST(VectorBinding, Basic) {
  std::vector<Model> models;
  ViewRegistry registry;

  TestVectorBinding::ModelAddedCallback added_callback =
      base::Bind(&OnModelAdded, base::Unretained(&registry));

  TestVectorBinding::ModelRemovedCallback removed_callback =
      base::Bind(&OnModelRemoved, base::Unretained(&registry));

  TestVectorBinding binding(&models, added_callback, removed_callback);

  models.push_back({42});

  EXPECT_EQ(0lu, registry.views().size());

  binding.Update();

  EXPECT_EQ(1lu, registry.views().size());

  EXPECT_EQ(42, registry.views().front()->value);

  models[0].value = 123;

  binding.Update();

  EXPECT_EQ(123, registry.views().front()->value);

  models.clear();
  models.push_back({246});

  // We have the same number of models as before, nevertheless, we should have
  // propagated the values for the models at each index of the vector, even if
  // they are new (since we reference elements by index).
  binding.Update();

  EXPECT_EQ(1lu, registry.views().size());
  EXPECT_EQ(246, registry.views().front()->value);

  models.clear();
  binding.Update();

  EXPECT_EQ(0lu, registry.views().size());
}

}  // namespace vr

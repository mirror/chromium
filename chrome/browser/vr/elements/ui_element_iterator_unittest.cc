// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/ui_element.h"

#include "base/memory/ptr_util.h"
#include "base/test/gtest_util.h"
#include "chrome/browser/vr/ui_scene.h"

namespace vr {

namespace {

// Constructs a tree of the following form
// 1 kRoot
//   2 k2dBrowsingRoot
//     3 kFloor
//     4 k2dBrowsingContentGroup
//       5 kBackplane
//       6 kContentQuad
//       7 kUrlBar
//     8 kCeiling
void MakeTree(UiScene* scene) {
  auto element = base::MakeUnique<UiElement>();
  element->set_name(k2dBrowsingRoot);
  scene->AddUiElement(kRoot, std::move(element));

  element = base::MakeUnique<UiElement>();
  element->set_name(kFloor);
  scene->AddUiElement(k2dBrowsingRoot, std::move(element));

  element = base::MakeUnique<UiElement>();
  element->set_name(k2dBrowsingContentGroup);
  scene->AddUiElement(k2dBrowsingRoot, std::move(element));

  element = base::MakeUnique<UiElement>();
  element->set_name(kBackplane);
  scene->AddUiElement(k2dBrowsingContentGroup, std::move(element));

  element = base::MakeUnique<UiElement>();
  element->set_name(kContentQuad);
  scene->AddUiElement(k2dBrowsingContentGroup, std::move(element));

  element = base::MakeUnique<UiElement>();
  element->set_name(kUrlBar);
  scene->AddUiElement(k2dBrowsingContentGroup, std::move(element));

  element = base::MakeUnique<UiElement>();
  element->set_name(kCeiling);
  scene->AddUiElement(k2dBrowsingRoot, std::move(element));
}

template <typename T>
void CollectElements(T* e, std::vector<T*>* elements) {
  elements->push_back(e);
  for (auto& child : e->children()) {
    CollectElements(child.get(), elements);
  }
}

}  // namespace

TEST(UiElementIterator, Forward) {
  UiScene scene;
  MakeTree(&scene);
  std::vector<UiElement*> elements;
  CollectElements(&scene.root_element(), &elements);
  size_t i = 0;
  for (auto& e : scene) {
    EXPECT_EQ(elements[i++]->id(), e.id());
  }
  EXPECT_EQ(elements.size(), i);

  i = 0;
  for (auto& e : const_cast<const UiScene&>(scene)) {
    EXPECT_EQ(elements[i++]->id(), e.id());
  }
  EXPECT_EQ(elements.size(), i);
}

TEST(UiElementIterator, ForwardSubtree) {
  UiScene scene;
  MakeTree(&scene);
  std::vector<UiElement*> elements;
  CollectElements(scene.GetUiElementByName(k2dBrowsingContentGroup), &elements);
  size_t i = 0;
  for (auto& e : *scene.GetUiElementByName(k2dBrowsingContentGroup)) {
    EXPECT_EQ(elements[i++]->id(), e.id());
  }
  EXPECT_EQ(elements.size(), i);

  i = 0;
  for (auto& e : *const_cast<const UiElement*>(
           scene.GetUiElementByName(k2dBrowsingContentGroup))) {
    EXPECT_EQ(elements[i++]->id(), e.id());
  }
  EXPECT_EQ(elements.size(), i);
}

TEST(UiElementIterator, ForwardLeaf) {
  UiScene scene;
  MakeTree(&scene);
  std::vector<UiElement*> elements;
  CollectElements(scene.GetUiElementByName(kCeiling), &elements);
  size_t i = 0;
  for (auto& e : *scene.GetUiElementByName(kCeiling)) {
    EXPECT_EQ(elements[i++]->id(), e.id());
  }
  EXPECT_EQ(elements.size(), i);
  EXPECT_EQ(1lu, i);

  i = 0;
  for (auto& e :
       *const_cast<const UiElement*>(scene.GetUiElementByName(kCeiling))) {
    EXPECT_EQ(elements[i++]->id(), e.id());
  }
  EXPECT_EQ(elements.size(), i);
  EXPECT_EQ(1lu, i);
}

TEST(UiElementIterator, Reverse) {
  UiScene scene;
  MakeTree(&scene);
  std::vector<UiElement*> elements;
  CollectElements(&scene.root_element(), &elements);
  size_t i = 0;
  for (auto& e : scene.reversed()) {
    EXPECT_EQ(elements[elements.size() - i++ - 1lu]->id(), e.id());
  }
  EXPECT_EQ(elements.size(), i);

  i = 0;
  for (auto& e : const_cast<const UiScene&>(scene).reversed()) {
    EXPECT_EQ(elements[elements.size() - i++ - 1lu]->id(), e.id());
  }
  EXPECT_EQ(elements.size(), i);
}

TEST(UiElementIterator, ReverseSubtree) {
  UiScene scene;
  MakeTree(&scene);
  std::vector<UiElement*> elements;
  CollectElements(scene.GetUiElementByName(k2dBrowsingContentGroup), &elements);
  size_t i = 0;
  for (auto& e :
       scene.GetUiElementByName(k2dBrowsingContentGroup)->reversed()) {
    EXPECT_EQ(elements[elements.size() - i++ - 1lu]->id(), e.id());
  }
  EXPECT_EQ(elements.size(), i);

  i = 0;
  for (auto& e : const_cast<const UiElement*>(
                     scene.GetUiElementByName(k2dBrowsingContentGroup))
                     ->reversed()) {
    EXPECT_EQ(elements[elements.size() - i++ - 1lu]->id(), e.id());
  }
  EXPECT_EQ(elements.size(), i);
}

TEST(UiElementIterator, ReverseLeaf) {
  UiScene scene;
  MakeTree(&scene);
  std::vector<UiElement*> elements;
  CollectElements(scene.GetUiElementByName(kCeiling), &elements);
  size_t i = 0;
  for (auto& e : scene.GetUiElementByName(kCeiling)->reversed()) {
    EXPECT_EQ(elements[elements.size() - i++ - 1lu]->id(), e.id());
  }
  EXPECT_EQ(elements.size(), i);
  EXPECT_EQ(1lu, i);

  i = 0;
  for (auto& e :
       const_cast<const UiElement*>(scene.GetUiElementByName(kCeiling))
           ->reversed()) {
    EXPECT_EQ(elements[elements.size() - i++ - 1lu]->id(), e.id());
  }
  EXPECT_EQ(elements.size(), i);
  EXPECT_EQ(1lu, i);
}

}  // namespace vr

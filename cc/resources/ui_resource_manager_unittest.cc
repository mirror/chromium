// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/ui_resource_manager.h"

#include "base/memory/ptr_util.h"
#include "cc/test/fake_scoped_ui_resource.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

TEST(UIResourceManagerTest, CreateDeleteResource) {
  auto resource_manager = base::MakeUnique<UIResourceManager>();
  auto ui_resource = FakeScopedUIResource::Create(resource_manager.get());
  ui_resource->DeleteResource();
  auto requests = resource_manager->TakeUIResourcesRequests();
  EXPECT_TRUE(requests.empty());
}

}  // namespace
}  // namespace cc

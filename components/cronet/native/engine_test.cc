// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cronet_c.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "components/cronet/native/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

class EngineTest : public ::testing::Test {
 protected:
  EngineTest() = default;
  ~EngineTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(EngineTest);
};

namespace {}  // namespace

TEST_F(EngineTest, StartCronetEngine) {
  Cronet_EnginePtr engine = Cronet_Engine_Create();
  Cronet_EngineParamsPtr engine_params = Cronet_EngineParams_Create();
  Cronet_EngineParams_set_userAgent(engine_params, "test");
  Cronet_Engine_StartWithParams(engine, engine_params);
  Cronet_Engine_Destroy(engine);
}

// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_SHARED_IMPL_TEST_GLOBALS_H_
#define PPAPI_SHARED_IMPL_TEST_GLOBALS_H_

#include "base/compiler_specific.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/resource_tracker.h"
#include "ppapi/shared_impl/var_tracker.h"

namespace ppapi {

// Implementation of PpapiGlobals for tests that don't need either the host- or
// plugin-specific implementations.
class TestGlobals : public PpapiGlobals {
 public:
  TestGlobals();
  virtual ~TestGlobals();

  // PpapiGlobals implementation.
  virtual ResourceTracker* GetResourceTracker() OVERRIDE;
  virtual VarTracker* GetVarTracker() OVERRIDE;
  virtual FunctionGroupBase* GetFunctionAPI(PP_Instance inst,
                                            ApiID id) OVERRIDE;
  virtual PP_Module GetModuleForInstance(PP_Instance instance) OVERRIDE;

 private:
  ResourceTracker resource_tracker_;
  VarTracker var_tracker_;

  DISALLOW_COPY_AND_ASSIGN(TestGlobals);
};

}  // namespace ppapi

#endif   // PPAPI_SHARED_IMPL_TEST_GLOBALS_H_

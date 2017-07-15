// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/xcode_object.h"

#include "base/memory/ptr_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// Tests that instantiating Xcode objects doesn't crash.
TEST(XcodeObject, ClassInstantiation) {
  PBXSourcesBuildPhase pbx_sources_build_phase;
  PBXFrameworksBuildPhase pbx_frameworks_build_phase;
  PBXShellScriptBuildPhase pbx_shell_script_build_phase("name", "shell_script");

  PBXGroup pbx_group("path", "name");
  PBXProject pbx_project("name", "config_name", "source_path", PBXAttributes());

  PBXFileReference pbx_file_reference("name", "path", "type");
  PBXBuildFile pbx_build_file(&pbx_file_reference, &pbx_sources_build_phase,
                              CompilerFlags::NONE);

  PBXAggregateTarget pbx_aggregate_target("name", "shell_script", "config_name",
                                          PBXAttributes());
  PBXNativeTarget pbx_native_target("name", "shell_script", "config_name",
                                    PBXAttributes(), "product_type",
                                    "product_name", &pbx_file_reference);
  PBXContainerItemProxy pbx_container_item_proxy(&pbx_project,
                                                 &pbx_native_target);
  PBXTargetDependency pbx_target_dependency(
      &pbx_native_target, base::MakeUnique<PBXContainerItemProxy>(
                              &pbx_project, &pbx_native_target));

  XCBuildConfiguration xc_build_configuration("name", PBXAttributes());
  XCConfigurationList xc_xcconfiguration_list("name", PBXAttributes(),
                                              &pbx_native_target);
}

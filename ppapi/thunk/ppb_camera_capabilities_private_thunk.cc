// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// From private/ppb_camera_capabilities_private.idl modified Tue Feb  3 19:54:34
// 2015.

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_camera_capabilities_private.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppapi_thunk_export.h"
#include "ppapi/thunk/ppb_camera_capabilities_api.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Bool IsCameraCapabilities(PP_Resource resource) {
  VLOG(4) << "PPB_CameraCapabilities_Private::IsCameraCapabilities()";
  EnterResource<PPB_CameraCapabilities_API> enter(resource, false);
  return PP_FromBool(enter.succeeded());
}

void GetSupportedPreviewSizes(PP_Resource capabilities,
                              int32_t* array_size,
                              struct PP_Size** preview_sizes) {
  VLOG(4) << "PPB_CameraCapabilities_Private::GetSupportedPreviewSizes()";
  EnterResource<PPB_CameraCapabilities_API> enter(capabilities, true);
  if (enter.failed())
    return;
  enter.object()->GetSupportedPreviewSizes(array_size, preview_sizes);
}

const PPB_CameraCapabilities_Private_0_1
    g_ppb_cameracapabilities_private_thunk_0_1 = {&IsCameraCapabilities,
                                                  &GetSupportedPreviewSizes};

}  // namespace

PPAPI_THUNK_EXPORT const PPB_CameraCapabilities_Private_0_1*
GetPPB_CameraCapabilities_Private_0_1_Thunk() {
  return &g_ppb_cameracapabilities_private_thunk_0_1;
}

}  // namespace thunk
}  // namespace ppapi

/* Copyright 2014 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_camera_capabilities_private.idl,
 *   modified Tue Feb  3 19:54:34 2015.
 */

#ifndef PPAPI_C_PRIVATE_PPB_CAMERA_CAPABILITIES_PRIVATE_H_
#define PPAPI_C_PRIVATE_PPB_CAMERA_CAPABILITIES_PRIVATE_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/pp_stdint.h"

#define PPB_CAMERACAPABILITIES_PRIVATE_INTERFACE_0_1 \
    "PPB_CameraCapabilities_Private;0.1"
#define PPB_CAMERACAPABILITIES_PRIVATE_INTERFACE \
    PPB_CAMERACAPABILITIES_PRIVATE_INTERFACE_0_1

/**
 * @file
 * This file defines the PPB_CameraCapabilities_Private interface for
 * establishing an image capture configuration resource within the browser.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * The <code>PPB_CameraCapabilities_Private</code> interface contains pointers
 * to several functions for getting the image capture capabilities within the
 * browser.
 */
struct PPB_CameraCapabilities_Private_0_1 {
  /**
   * IsCameraCapabilities() determines if the given resource is a
   * <code>PPB_CameraCapabilities_Private</code>.
   *
   * @param[in] resource A <code>PP_Resource</code> corresponding to an image
   * capture capabilities resource.
   *
   * @return A <code>PP_Bool</code> containing <code>PP_TRUE</code> if the given
   * resource is an <code>PP_CameraCapabilities_Private</code> resource,
   * otherwise <code>PP_FALSE</code>.
   */
  PP_Bool (*IsCameraCapabilities)(PP_Resource resource);
  /**
   * GetSupportedPreviewSizes() returns the supported preview sizes for the
   * given <code>PPB_CameraCapabilities_Private</code>.
   *
   * @param[in] capabilities A <code>PP_Resource</code> corresponding to an
   * image capture capabilities resource.
   * @param[out] array_size The size of preview size array.
   * @param[out] preview_sizes An array of <code>PP_Size</code> corresponding
   * to the supported preview sizes in pixels. The ownership of the array
   * belongs to <code>PPB_CameraCapabilities_Private</code> and the caller
   * should not free it. When a PPB_CameraCapabilities_Private is deleted,
   * the array returning from this is no longer valid.
   */
  void (*GetSupportedPreviewSizes)(PP_Resource capabilities,
                                   int32_t* array_size,
                                   struct PP_Size** preview_sizes);
};

typedef struct PPB_CameraCapabilities_Private_0_1
    PPB_CameraCapabilities_Private;
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_CAMERA_CAPABILITIES_PRIVATE_H_ */


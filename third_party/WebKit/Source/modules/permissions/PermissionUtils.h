// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_MODULES_PERMISSIONS_PERMISSIONUTILS_H_
#define THIRD_PARTY_WEBKIT_SOURCE_MODULES_PERMISSIONS_PERMISSIONUTILS_H_

#include "public/platform/modules/permissions/permission.mojom-blink.h"

namespace blink {

class ExecutionContext;

bool ConnectToPermissionService(ExecutionContext*,
                                mojom::blink::PermissionServiceRequest);

mojom::blink::PermissionDescriptorPtr CreatePermissionDescriptor(
    mojom::blink::PermissionName);

mojom::blink::PermissionDescriptorPtr CreateMidiPermissionDescriptor(
    bool sysex);

mojom::blink::PermissionDescriptorPtr CreateClipboardReadPermissionDescriptor(
    bool allowWithoutGesture);

mojom::blink::PermissionDescriptorPtr CreateClipboardWritePermissionDescriptor(
    bool allowWithoutGesture);

}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_MODULES_PERMISSIONS_PERMISSIONUTILS_H_

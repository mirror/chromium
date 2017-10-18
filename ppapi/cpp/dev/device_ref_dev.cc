// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/dev/device_ref_dev.h"

#include "ppapi/cpp/module_impl.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_DeviceRef_Dev_0_1>() {
  return PPB_DEVICEREF_DEV_INTERFACE_0_1;
}

template <> const char* interface_name<PPB_DeviceRef_Dev_0_2>() {
  return PPB_DEVICEREF_DEV_INTERFACE_0_2;
}

}  // namespace

DeviceRef_Dev::DeviceRef_Dev() {
}

DeviceRef_Dev::DeviceRef_Dev(PP_Resource resource) : Resource(resource) {
}

DeviceRef_Dev::DeviceRef_Dev(PassRef, PP_Resource resource)
    : Resource(PASS_REF, resource) {
}

DeviceRef_Dev::DeviceRef_Dev(const DeviceRef_Dev& other) : Resource(other) {
}

DeviceRef_Dev::~DeviceRef_Dev() {
}

PP_DeviceType_Dev DeviceRef_Dev::GetType() const {
  if (has_interface<PPB_DeviceRef_Dev_0_2>()) {
    return get_interface<PPB_DeviceRef_Dev_0_2>()->GetType(pp_resource());
  } else if (has_interface<PPB_DeviceRef_Dev_0_1>()) {
    return get_interface<PPB_DeviceRef_Dev_0_1>()->GetType(pp_resource());
  }
  return PP_DEVICETYPE_DEV_INVALID;
}

Var DeviceRef_Dev::GetName() const {
  if (has_interface<PPB_DeviceRef_Dev_0_2>()) {
    return Var(PASS_REF,
               get_interface<PPB_DeviceRef_Dev_0_2>()->GetName(pp_resource()));
  } else if (has_interface<PPB_DeviceRef_Dev_0_1>()) {
    return Var(PASS_REF,
               get_interface<PPB_DeviceRef_Dev_0_1>()->GetName(pp_resource()));
  }
  return Var();
}

Var DeviceRef_Dev::GetId() const {
  if (has_interface<PPB_DeviceRef_Dev_0_2>()) {
    return Var(PASS_REF,
               get_interface<PPB_DeviceRef_Dev_0_2>()->GetId(pp_resource()));
  }
  return Var();
}

}  // namespace pp

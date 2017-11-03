// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef CONTENT_PUBLIC_COMMON_WEBPLUGININFO_PARAM_TRAITS_H_
#define CONTENT_PUBLIC_COMMON_WEBPLUGININFO_PARAM_TRAITS_H_

#include "content/public/common/webplugininfo.h"
#include "ipc/ipc_message_macros.h"

IPC_STRUCT_TRAITS_BEGIN(content::WebPluginMimeType)
  IPC_STRUCT_TRAITS_MEMBER(mime_type)
  IPC_STRUCT_TRAITS_MEMBER(file_extensions)
  IPC_STRUCT_TRAITS_MEMBER(description)
  IPC_STRUCT_TRAITS_MEMBER(additional_param_names)
  IPC_STRUCT_TRAITS_MEMBER(additional_param_values)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(content::WebPluginInfo)
  IPC_STRUCT_TRAITS_MEMBER(name)
  IPC_STRUCT_TRAITS_MEMBER(path)
  IPC_STRUCT_TRAITS_MEMBER(version)
  IPC_STRUCT_TRAITS_MEMBER(desc)
  IPC_STRUCT_TRAITS_MEMBER(mime_types)
  IPC_STRUCT_TRAITS_MEMBER(type)
  IPC_STRUCT_TRAITS_MEMBER(pepper_permissions)
IPC_STRUCT_TRAITS_END()

#endif  // CONTENT_PUBLIC_COMMON_WEBPLUGININFO_PARAM_TRAITS_H_

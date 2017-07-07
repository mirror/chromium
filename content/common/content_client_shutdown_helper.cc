// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/content_client_shutdown_helper.h"

namespace content {

template <>
ContentBrowserClient**
ContentClientShutdownHelper::GetStorage<ContentBrowserClient>() {
  return GetContentClient() ? &GetContentClient()->browser_ : nullptr;
}

template <>
ContentRendererClient**
ContentClientShutdownHelper::GetStorage<ContentRendererClient>() {
  return GetContentClient() ? &GetContentClient()->renderer_ : nullptr;
}

template <>
ContentGpuClient** ContentClientShutdownHelper::GetStorage<ContentGpuClient>() {
  return GetContentClient() ? &GetContentClient()->gpu_ : nullptr;
}

template <>
ContentUtilityClient**
ContentClientShutdownHelper::GetStorage<ContentUtilityClient>() {
  return GetContentClient() ? &GetContentClient()->utility_ : nullptr;
}

}  // namespace content

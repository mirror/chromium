// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/headless_content_utility_client.h"

#include "printing/features/features.h"

#if BUILDFLAG(ENABLE_BASIC_PRINTING)
#include "components/printing/service/public/cpp/pdf_compositor_service_factory.h"
#include "components/printing/service/public/interfaces/pdf_compositor.mojom.h"
#endif

namespace headless {

HeadlessContentUtilityClient::HeadlessContentUtilityClient() {}

HeadlessContentUtilityClient::~HeadlessContentUtilityClient() {}

void HeadlessContentUtilityClient::RegisterServices(
    HeadlessContentUtilityClient::StaticServiceMap* services) {
#if BUILDFLAG(ENABLE_BASIC_PRINTING)
  service_manager::EmbeddedServiceInfo pdf_compositor_info;
  pdf_compositor_info.factory = base::Bind(
      &printing::CreatePdfCompositorService, "");  // GetUserAgent());
  services->emplace(printing::mojom::kServiceName, pdf_compositor_info);
#endif
}

}  // namespace headless

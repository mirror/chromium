// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/printing/service/public/cpp/pdf_compositor_service_factory.h"

#include "build/build_config.h"
#include "components/printing/service/pdf_compositor_service.h"
#include "content/public/utility/utility_thread.h"
#include "third_party/WebKit/public/platform/WebImageGenerator.h"
#include "third_party/skia/include/core/SkGraphics.h"

#if defined(OS_WIN)
#include "content/child/dwrite_font_proxy/dwrite_font_proxy_init_win.h"
#endif

namespace printing {

std::unique_ptr<service_manager::Service> CreatePdfCompositorService(
    const std::string& creator) {
  content::UtilityThread::Get()->EnsureBlinkInitialized();
#if defined(OS_WIN)
  // Initialize direct write font proxy so skia can use it.
  content::InitializeDWriteFontProxy();
#endif
  // Hook up blink's codecs so skia can call them.
  SkGraphics::SetImageGeneratorFromEncodedDataFactory(
      blink::WebImageGenerator::CreateAsSkImageGenerator);
  return printing::PdfCompositorService::Create(creator);
}

}  // namespace printing

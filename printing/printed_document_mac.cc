// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/printed_document.h"

#import <ApplicationServices/ApplicationServices.h>
#import <CoreFoundation/CoreFoundation.h>

#include "base/logging.h"
#include "printing/page_number.h"
#include "printing/printed_page.h"

namespace printing {

void PrintedDocument::Render(printing::NativeDrawingContext context) {
  DCHECK(context);

  const MetafilePlayer* metafile;
  {
    base::AutoLock lock(lock_);
    metafile = GetMetafile();
  }

  DCHECK(metafile);
  const PageSetup& page_setup = immutable_.settings_.page_setup_device_units();
  gfx::Rect content_area =
      page.GetCenteredPageContentRect(page_setup.physical_size());

  struct Metafile::MacRenderPageParams params;
  params.autorotate = true;
  // only lock once
  size_t num_pages = page_count();
  for (size_t metafile_page_number = 1; metafile_page_number <= num_pages;
       metafile_page_number++) {
    metafile->RenderPage(metafile_page_number, context, content_area.ToCGRect(),
                         params);
  }
}

}  // namespace printing

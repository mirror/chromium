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

void PrintedDocument::RenderPrintedPage(
    const PrintedPage& page,
    printing::NativeDrawingContext context) const {
  DCHECK(context);

  base::AutoLock lock(lock_);
#ifndef NDEBUG
  // Make sure the page is from our list.
  DCHECK(&page == mutable_.pages_.find(page.page_number() - 1)->second.get());

  // Make sure the first page exists.
  DCHECK_GE(mutable_.first_page, 0);
  DCHECK_NE(mutable_.first_page, INT_MAX);
  DCHECK(mutable_.pages_.find(mutable_.first_page)->second.get());
#endif

  const PageSetup& page_setup = immutable_.settings_.page_setup_device_units();
  gfx::Rect content_area =
      page.GetCenteredPageContentRect(page_setup.physical_size());

  // Always use the metafile from the first page.
  const MetafilePlayer* metafile =
      mutable_.pages_.find(mutable_.first_page)->second->metafile();
  struct Metafile::MacRenderPageParams params;
  params.autorotate = true;
  metafile->RenderPage(page.page_number(), context, content_area.ToCGRect(),
                       params);
}

}  // namespace printing

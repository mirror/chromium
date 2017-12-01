// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/printed_document.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "printing/page_number.h"
#include "printing/printed_page.h"
#include "printing/printing_context_linux.h"

#if defined(OS_ANDROID) || defined(OS_CHROMEOS)
#error "This file is not used on Android / ChromeOS"
#endif

namespace printing {

void PrintedDocument::Render(PrintingContext* context) {
  DCHECK(context);

  {
    base::AutoLock lock(lock_);
    const MetafilePlayer* metafile = GetMetafile();
    DCHECK(metafile);
    static_cast<PrintingContextLinux*>(context)->PrintDocument(*metafile);
  }
}

}  // namespace printing

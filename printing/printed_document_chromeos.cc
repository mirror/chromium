// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/printed_document.h"

#include "base/logging.h"
#include "printing/page_number.h"
#include "printing/printed_page.h"

#if defined(USE_CUPS)
#include "printing/printing_context_chromeos.h"
#endif

namespace printing {

void PrintedDocument::Render(PrintingContext* context) {
#if defined(USE_CUPS)
  DCHECK(context);

  {
    base::AutoLock lock(lock_);
    std::vector<char> buffer;
    const MetafilePlayer* metafile = GetMetafile();
    DCHECK(metafile);
    if (metafile->GetDataAsVector(&buffer)) {
      static_cast<PrintingContextChromeos*>(context)->StreamData(buffer);
    } else {
      LOG(WARNING) << "Failed to read data from metafile";
    }
  }
#else
  NOTREACHED();
#endif  // defined(USE_CUPS)
}

}  // namespace printing

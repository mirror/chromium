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

bool PrintedDocument::Render(PrintingContext* context) {
#if defined(USE_CUPS)
  DCHECK(context);

  if (context->NewPage() != PrintingContext::OK)
    return false;
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
  if (context->PageDone() != PrintingContext::OK)
    return false;
  return true;
#else
  NOTREACHED();
  return false;
#endif  // defined(USE_CUPS)
}

}  // namespace printing

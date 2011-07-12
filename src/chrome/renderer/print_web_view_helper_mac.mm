// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/print_web_view_helper.h"

#import <AppKit/AppKit.h>

#include "base/logging.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/metrics/histogram.h"
#include "chrome/common/print_messages.h"
#include "printing/metafile.h"
#include "printing/metafile_impl.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"

#if defined(USE_SKIA)
#include "printing/metafile_skia_wrapper.h"
#include "skia/ext/vector_canvas.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebCanvas.h"
#endif

using WebKit::WebFrame;

void PrintWebViewHelper::PrintPageInternal(
    const PrintMsg_PrintPage_Params& params,
    const gfx::Size& canvas_size,
    WebFrame* frame) {
  printing::NativeMetafile metafile;
  if (!metafile.Init())
    return;

  float scale_factor = frame->getPrintPageShrink(params.page_number);
  int page_number = params.page_number;

  // Render page for printing.
  gfx::Rect content_area(params.params.printable_size);
  RenderPage(params.params.printable_size, content_area, scale_factor,
      page_number, frame, &metafile);
  metafile.FinishDocument();

  PrintHostMsg_DidPrintPage_Params page_params;
  page_params.data_size = metafile.GetDataSize();
  page_params.page_number = page_number;
  page_params.document_cookie = params.params.document_cookie;
  page_params.actual_shrink = scale_factor;
  page_params.page_size = params.params.page_size;
  page_params.content_area = gfx::Rect(params.params.margin_left,
                                       params.params.margin_top,
                                       params.params.printable_size.width(),
                                       params.params.printable_size.height());

  // Ask the browser to create the shared memory for us.
  if (!CopyMetafileDataToSharedMem(&metafile,
                                   &(page_params.metafile_data_handle))) {
    page_params.data_size = 0;
  }

  Send(new PrintHostMsg_DidPrintPage(routing_id(), page_params));
}

bool PrintWebViewHelper::CreatePreviewDocument(
    const PrintMsg_PrintPages_Params& params, WebKit::WebFrame* frame,
    WebKit::WebNode* node) {
  if (!PreviewPageRendered(-1))
    return false;

  PrintMsg_Print_Params printParams = params.params;
  UpdatePrintableSizeInPrintParameters(frame, node, &printParams);

  PrepareFrameAndViewForPrint prep_frame_view(printParams,
                                              frame, node, frame->view());
  if (!PreviewPageRendered(-1))
    return false;

  preview_page_count_ = prep_frame_view.GetExpectedPageCount();
  if (!preview_page_count_)
    return false;
  Send(new PrintHostMsg_DidGetPreviewPageCount(routing_id(),
                                               preview_page_count_));

  printing::PreviewMetafile metafile;
  if (!metafile.Init())
    return false;

  float scale_factor = frame->getPrintPageShrink(0);
  gfx::Rect content_area(printParams.margin_left, printParams.margin_top,
                         printParams.printable_size.width(),
                         printParams.printable_size.height());

  base::TimeTicks begin_time = base::TimeTicks::Now();
  base::TimeTicks page_begin_time = begin_time;

  if (params.pages.empty()) {
    for (int i = 0; i < preview_page_count_; ++i) {
      RenderPage(printParams.page_size, content_area, scale_factor, i, frame,
                 &metafile);
      page_begin_time = ReportPreviewPageRenderTime(page_begin_time);
      if (!PreviewPageRendered(i))
        return false;
    }
  } else {
    for (size_t i = 0; i < params.pages.size(); ++i) {
      if (params.pages[i] >= preview_page_count_)
        break;
      RenderPage(printParams.page_size, content_area, scale_factor,
                 params.pages[i], frame, &metafile);
      page_begin_time = ReportPreviewPageRenderTime(page_begin_time);
      if (!PreviewPageRendered(params.pages[i]))
        return false;
    }
  }

  base::TimeDelta render_time = base::TimeTicks::Now() - begin_time;

  prep_frame_view.FinishPrinting();
  metafile.FinishDocument();

  ReportTotalPreviewGenerationTime(params.pages.size(),
                                   preview_page_count_,
                                   render_time,
                                   base::TimeTicks::Now() - begin_time);

  PrintHostMsg_DidPreviewDocument_Params preview_params;
  preview_params.reuse_existing_data = false;
  preview_params.data_size = metafile.GetDataSize();
  preview_params.document_cookie = params.params.document_cookie;
  preview_params.expected_pages_count = preview_page_count_;
  preview_params.modifiable = IsModifiable(frame, node);

  // Ask the browser to create the shared memory for us.
  if (!CopyMetafileDataToSharedMem(&metafile,
                                   &(preview_params.metafile_data_handle))) {
    return false;
  }
  Send(new PrintHostMsg_PagesReadyForPreview(routing_id(), preview_params));
  return true;
}

void PrintWebViewHelper::RenderPage(
    const gfx::Size& page_size, const gfx::Rect& content_area,
    const float& scale_factor, int page_number, WebFrame* frame,
    printing::Metafile* metafile) {

  {
#if defined(USE_SKIA)
    SkDevice* device = metafile->StartPageForVectorCanvas(
        page_size, content_area, scale_factor);
    if (!device)
      return;

    SkRefPtr<skia::VectorCanvas> canvas = new skia::VectorCanvas(device);
    canvas->unref();  // SkRefPtr and new both took a reference.
    WebKit::WebCanvas* canvasPtr = canvas.get();
    printing::MetafileSkiaWrapper::SetMetafileOnCanvas(canvasPtr, metafile);
#else
    bool success = metafile->StartPage(page_size, content_area, scale_factor);
    DCHECK(success);
    // printPage can create autoreleased references to |context|. PDF contexts
    // don't write all their data until they are destroyed, so we need to make
    // certain that there are no lingering references.
    base::mac::ScopedNSAutoreleasePool pool;
    CGContextRef cgContext = metafile->context();
    CGContextRef canvasPtr = cgContext;
#endif
    frame->printPage(page_number, canvasPtr);
  }

  // Done printing. Close the device context to retrieve the compiled metafile.
  metafile->FinishPage();
}

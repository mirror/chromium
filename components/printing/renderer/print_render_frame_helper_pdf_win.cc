// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/printing/renderer/print_render_frame_helper.h"

#include <stddef.h>

#include "base/logging.h"
#include "base/process/process_handle.h"
#include "components/printing/common/print_messages.h"
#include "printing/features/features.h"
#include "printing/metafile_skia_wrapper.h"

namespace printing {

#if BUILDFLAG(ENABLE_BASIC_PRINTING)
bool PrintRenderFrameHelper::PrintPagesNative(blink::WebLocalFrame* frame,
                                              int page_count) {
  const PrintMsg_PrintPages_Params& params = *print_pages_params_;
  std::vector<int> printed_pages = GetPrintedPages(params, page_count);
  if (printed_pages.empty())
    return false;

  std::vector<gfx::Size> page_size_in_dpi(printed_pages.size());
  std::vector<gfx::Rect> content_area_in_dpi(printed_pages.size());
  std::vector<gfx::Rect> printable_area_in_dpi(printed_pages.size());

  PdfMetafileSkia metafile(params.params.printed_doc_type);
  CHECK(metafile.Init());

  PrintMsg_PrintPages_Params page_params;
  page_params.params = params.params;

  int dpi_x = params.params.dpi.width();
  int dpi_y = params.params.dpi.height();
  int dpi = std::min(dpi_x, dpi_y);  // to convert size back correctly.
  for (size_t i = 0; i < printed_pages.size(); ++i) {
    PrintPageInternal(params.params, printed_pages[i], page_count, frame,
                      &metafile, &page_size_in_dpi[i], &content_area_in_dpi[i],
                      &printable_area_in_dpi[i]);
    // Scale the page size back to the size in DPI. Because Blink cannot scale
    // differently in different dimensions, we pass in the page size based on
    // the minimum dpi for in printing/print_settings_initializer_win.cc. Need
    // to now scale it back for the printer. Note: if dpi_x == dpi_y (true for
    // most printers), this has no effect.
    page_size_in_dpi[i] = gfx::Size(page_size_in_dpi[i].width() * dpi_x / dpi,
                                    page_size_in_dpi[i].height() * dpi_y / dpi);
    content_area_in_dpi[i] =
        gfx::Rect(content_area_in_dpi[i].x() * dpi_x / dpi,
                  content_area_in_dpi[i].y() * dpi_y / dpi,
                  content_area_in_dpi[i].width() * dpi_x / dpi,
                  content_area_in_dpi[i].height() * dpi_y / dpi);
    printable_area_in_dpi[i] =
        gfx::Rect(printable_area_in_dpi[i].x() * dpi_x / dpi,
                  printable_area_in_dpi[i].y() * dpi_y / dpi,
                  printable_area_in_dpi[i].width() * dpi_x / dpi,
                  printable_area_in_dpi[i].height() * dpi_y / dpi);
  }

  // blink::printEnd() for PDF should be called before metafile is closed.
  FinishFramePrinting();

  metafile.FinishDocument();

  PrintHostMsg_DidPrintPage_Params printed_page_params;
  if (!CopyMetafileDataToSharedMem(metafile,
                                   &printed_page_params.metafile_data_handle)) {
    return false;
  }

  printed_page_params.content_area = params.params.printable_area;
  printed_page_params.data_size = metafile.GetDataSize();
  printed_page_params.document_cookie = params.params.document_cookie;
  printed_page_params.page_size = params.params.page_size;

  for (size_t i = 0; i < printed_pages.size(); ++i) {
    printed_page_params.page_number = printed_pages[i];
    printed_page_params.page_size = page_size_in_dpi[i];
    printed_page_params.content_area = content_area_in_dpi[i];
    printed_page_params.physical_offsets =
        gfx::Point(printable_area_in_dpi[i].x(), printable_area_in_dpi[i].y());
    Send(new PrintHostMsg_DidPrintPage(routing_id(), printed_page_params));
    // Send the rest of the pages with an invalid metafile handle.
    // TODO(erikchen): Fix semantics. See https://crbug.com/640840
    if (printed_page_params.metafile_data_handle.IsValid())
      printed_page_params.metafile_data_handle = base::SharedMemoryHandle();
  }
  return true;
}
#endif  // BUILDFLAG(ENABLE_BASIC_PRINTING)

}  // namespace printing
